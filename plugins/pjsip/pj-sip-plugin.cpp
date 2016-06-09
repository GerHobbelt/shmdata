/*
 * This file is part of switcher-pjsip.
 *
 * switcher-pjsip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * switcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with switcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "./pj-sip-plugin.hpp"
#include "switcher/net-utils.hpp"
#include "switcher/std2.hpp"

namespace switcher {
SWITCHER_DECLARE_PLUGIN(SIPPlugin);
SWITCHER_MAKE_QUIDDITY_DOCUMENTATION(SIPPlugin,
                                     "sip",
                                     "SIP (Session Initiation Protocol)",
                                     "network",
                                     "writer",
                                     "Manages user sessions",
                                     "LGPL",
                                     "Nicolas Bouillot");

SIPPlugin* SIPPlugin::this_ = nullptr;

std::atomic<unsigned short> SIPPlugin::sip_plugin_used_(0);

SIPPlugin::SIPPlugin(const std::string&)
    : port_id_(pmanage<MPtr(&PContainer::make_unsigned_int)>(
          "port",
          [this](const unsigned int& val) {
            if (val == sip_port_) return true;
            sip_port_ = val;
            return pjsip_->run<bool>(
                [this]() { return start_sip_transport(); });
          },
          [this]() { return sip_port_; },
          "SIP Port",
          "SIP port used when registering",
          sip_port_,
          0u,
          65535u)) {}

SIPPlugin::~SIPPlugin() {
  if (!i_m_the_one_) return;

  sip_calls_->finalize_calls();
  sip_calls_.reset(nullptr);

  pjsip_->run([this]() {
    stun_turn_.reset(nullptr);
    sip_presence_.reset(nullptr);
  });

  pjsip_.reset(nullptr);
  this_ = nullptr;
  i_m_the_one_ = false;
  sip_plugin_used_ = 0;
}

bool SIPPlugin::init() {
  if (1 == sip_plugin_used_.fetch_or(1)) {
    g_warning("an other sip quiddity is instancied, cannot init");
    return false;
  }
  i_m_the_one_ = true;
  this_ = this;

  pjsip_ = std2::make_unique<ThreadedWrapper<PJSIP>>(
      // init
      [&]() {
        start_sip_transport();
        sip_calls_ = std2::make_unique<PJCall>();
        sip_presence_ = std2::make_unique<PJPresence>();
        stun_turn_ = std2::make_unique<PJStunTurn>();
        return true;
      },
      // destruct
      [&]() {
        sip_calls_.reset(nullptr);
        sip_presence_.reset(nullptr);
        stun_turn_.reset(nullptr);
      });

  if (!pjsip_->invoke<MPtr(&PJSIP::safe_bool_idiom)>()) return false;
  apply_configuration();
  return true;
}

void SIPPlugin::apply_configuration() {
  // trying to set port if configuration found
  if (config<MPtr(&InfoTree::branch_has_data)>("port")) {
    auto port = config<MPtr(&InfoTree::branch_get_value)>("port");
    if (pmanage<MPtr(&PContainer::set<unsigned int>)>(
            port_id_, port.copy_as<unsigned int>()))
      g_message("sip has set port from configuration");
    else
      g_warning("sip failed setting port from configuration");
  }

  // trying to set stun/turn from configuration
  std::string stun = config<MPtr(&InfoTree::branch_get_value)>("stun");
  std::string turn = config<MPtr(&InfoTree::branch_get_value)>("turn");
  std::string turn_user =
      config<MPtr(&InfoTree::branch_get_value)>("turn_user");
  std::string turn_pass =
      config<MPtr(&InfoTree::branch_get_value)>("turn_pass");
  if (!stun.empty()) {
    pjsip_->run([&]() {
      if (PJStunTurn::set_stun_turn(stun.c_str(),
                                    turn.c_str(),
                                    turn_user.c_str(),
                                    turn_pass.c_str(),
                                    stun_turn_.get())) {
        g_message("sip has set STUN/TURN from configuration");
      } else {
        g_warning("sip failed setting STUN/TURN from configuration");
      }
    });
  }

  // trying to register if a user is given
  std::string user = config<MPtr(&InfoTree::branch_get_value)>("user");
  if (!user.empty()) {
    std::string pass = config<MPtr(&InfoTree::branch_get_value)>("pass");
    pjsip_->run([&]() { sip_presence_->register_account(user, pass); });
    if (sip_presence_->registered_)
      g_message("sip registered using configuration file");
    else
      g_warning("sip failed registration from configuration");
  }
}

bool SIPPlugin::start_sip_transport() {
  if (-1 != transport_id_) pjsua_transport_close(transport_id_, PJ_FALSE);
  if (NetUtils::is_used(sip_port_)) {
    g_warning("SIP port cannot be binded (%u)", sip_port_);
    return false;
  }
  pjsua_transport_config cfg;
  pjsua_transport_config_default(&cfg);
  cfg.port = sip_port_;
  pj_status_t status =
      pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, &transport_id_);
  if (status != PJ_SUCCESS) {
    g_warning("Error creating transport");
    return false;
  }
  return true;
}

}  // namespace switcher