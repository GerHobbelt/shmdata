/*
 * This file is part of libswitcher.
 *
 * libswitcher is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __SWITCHER_PROPERTY_SPECIFICATION_H__
#define __SWITCHER_PROPERTY_SPECIFICATION_H__

#include <glib.h>  //log

#include <typeinfo>
#include <string>
#include <sstream>
#include <functional>

#include "./type-name-registry.hpp"
#include "./information-tree.hpp"
#include "./selection.hpp"
#include "./group.hpp"
#include "./fraction.hpp"
#include "./templated-sequence.hpp"

namespace switcher {

template<typename T, typename TT = T>
class PropertySpecification{
 public:
  PropertySpecification() = delete;

  // typename V is here in order to allow default value to be initialized from an other type,
  // e.g. an unsigned int with an int
  
  template <typename U = T, typename V>
  PropertySpecification(bool is_writable,
                        const std::string &label,
                        const std::string &description,
                        const V &default_value,
                        typename std::enable_if<
                        !std::is_arithmetic<U>::value 
                        >::type* = nullptr):
      spec_(data::Tree::make()),
      is_valid_([](const V &){return true;}){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make(TypeNameRegistry::get_name<U>()));
    spec_->graft("writable", data::Tree::make(is_writable));
    spec_->graft("default", data::Tree::make(static_cast<U>(default_value)));
  }

  template <typename U = T, typename V>
  PropertySpecification(bool is_writable,
                        const std::string &label,
                        const std::string &description,
                        const V &default_value,
                        const V &min_value = std::numeric_limits<U>::min(),
                        const V &max_value = std::numeric_limits<U>::max(),
                        typename std::enable_if<
                        !std::is_same<U, bool>::value && 
                        std::is_arithmetic<U>::value 
                        >::type* = nullptr):
  spec_(data::Tree::make()),
    is_valid_([min_value, max_value](const V &val){
        if (val < min_value || val > max_value){
          g_warning("value %s is out of range [%s,%s]",
                    std::to_string(val).c_str(),
                    std::to_string(min_value).c_str(),
                    std::to_string(max_value).c_str());
          return false;
        }
        return true;
      }){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make(TypeNameRegistry::get_name<U>()));
    spec_->graft("writable", data::Tree::make(is_writable));
    spec_->graft("default", data::Tree::make(static_cast<U>(default_value)));
    spec_->graft("min", data::Tree::make(static_cast<U>(min_value)));
    spec_->graft("max", data::Tree::make(static_cast<U>(max_value)));
  }
  
  template<typename U = bool>
  PropertySpecification(bool is_writable,
                        const std::string &label,
                        const std::string &description,
                        const bool &default_value):
      spec_(data::Tree::make()),
      is_valid_([](const bool &){return true;}){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make(TypeNameRegistry::get_name<bool>()));
    spec_->graft("writable", data::Tree::make(is_writable));
    spec_->graft("default", data::Tree::make(default_value));
  }

  template<typename U = char>
  PropertySpecification(bool is_writable,
                        const std::string &label,
                        const std::string &description,
                        const char &default_value):
      spec_(data::Tree::make()),
      is_valid_([](const char &){return true;}){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make(TypeNameRegistry::get_name<char>()));
    spec_->graft("writable", data::Tree::make(is_writable));
    spec_->graft("default", data::Tree::make(default_value));
  }

  template<typename U = Selection, typename V = Selection::index_t>
  PropertySpecification(bool is_writable,
                        const std::string &label,
                        const std::string &description,
                        const Selection &default_value,
                        Selection::index_t max):
      spec_(data::Tree::make()),
      is_valid_([max](const Selection::index_t &index){
          if (index > max){
            g_warning("selection index out of range");
            return false;
          }
          return true;}){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make("selection"));
    spec_->graft("writable", data::Tree::make(is_writable));
    spec_->graft("default", data::Tree::make(default_value.get()));
    size_t pos = 0;
    for (const auto &it: default_value.get_list()){
      auto tree = data::Tree::make();
      tree->graft(".label", data::Tree::make(it));
      tree->graft(".id", data::Tree::make(pos));  // overhiding id set by json serializer
      spec_->graft(".values." + std::to_string(pos), tree);
      ++pos;
    }
    spec_->tag_as_array(".values.", true);
  }

  template<typename U = Fraction>
  PropertySpecification(bool is_writable,
                        const std::string &label,
                        const std::string &description,
                        const Fraction &default_value,
                        Fraction::ator_t min_num,
                        Fraction::ator_t min_denom,
                        Fraction::ator_t max_num,
                        Fraction::ator_t max_denom):
      spec_(data::Tree::make()),
      is_valid_([min_num, min_denom, max_num, max_denom](const Fraction &frac){
          auto num = frac.numerator();
          if (num < min_num || num > max_num){
            g_warning("numerator out of range");
            return false;
          }
          auto denom = frac.denominator();
          if (denom < min_denom || denom > max_denom){
             g_warning("denominator out of range");
             return false;
          }
          return true;
        }){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make("fraction"));
    spec_->graft("writable", data::Tree::make(is_writable));
    spec_->graft("default numerator", data::Tree::make(default_value.numerator()));
    spec_->graft("default denominator", data::Tree::make(default_value.denominator()));
    spec_->graft("min numerator", data::Tree::make(min_num));
    spec_->graft("min denominator", data::Tree::make(min_denom));
    spec_->graft("max numerator", data::Tree::make(max_num));
    spec_->graft("max denominator", data::Tree::make(max_denom));
  }


  template<typename ...TupleParams>
  PropertySpecification(bool is_writable,
                        const std::string &label,
                        const std::string &description,
                        const std::tuple<TupleParams...> &default_value):
      spec_(data::Tree::make()),
      is_valid_([](const std::tuple<TupleParams...> &){return true;}){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make("tuple"));
    spec_->graft("writable", data::Tree::make(is_writable));
    print_tuple(".default.", default_value);
    spec_->tag_as_array(".default.", true);
  }

  template<typename U = Group,
            typename std::enable_if<std::is_same<U, Group>::value>::type* = nullptr>
  PropertySpecification(const std::string &label,
                        const std::string &description):
      spec_(data::Tree::make()),
      is_valid_([](const Group&){return false;}){
    spec_->graft("label", data::Tree::make(label));
    spec_->graft("description", data::Tree::make(description));
    spec_->graft("type", data::Tree::make("group"));
  }

  data::Tree::ptr get_spec(){
    return spec_;
  }

  bool is_valid(const TT &val) const{return is_valid_(val);}
  
 private:
  data::Tree::ptr spec_;
  const std::function<bool(const TT&)> is_valid_;
  
  // writing tuple:
  void print_targs(const std::string &, size_t){}
  template<typename F, typename ...U>
  void print_targs(const std::string &key, size_t pos, F first, U... args){
    auto tree = data::Tree::make();
    tree->graft(".id", data::Tree::make(pos));  // overhiding id set by json serializer
    tree->graft(".value", data::Tree::make(first));
    tree->graft(".type", data::Tree::make(TypeNameRegistry::get_name<F>()));
    spec_->graft(key + "." + std::to_string(pos), tree);
    print_targs(std::forward<const std::string &>(key), pos + 1, args...);
  }
  template <typename ...U, int ...S>
  void print_tuple_call(const std::string &key, const std::tuple<U...> &tup, tseq<S...>){
    print_targs(std::forward<const std::string &>(key), 0, std::get<S>(tup)...);
  }
  template <typename ...U>
  void print_tuple(const std::string &key, const std::tuple<U...> &tup){
    print_tuple_call(std::forward<const std::string &>(key),
                   std::forward<const std::tuple<U...> &>(tup),
                   typename gens<sizeof...(U)>::type());
  }
  
};

}  // namespace switcher
#endif
