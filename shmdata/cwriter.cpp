/*
 * Copyright (C) 2015 Nicolas Bouillot (http://www.nicolasbouillot.net)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include "./cwriter.h"
#include "./writer.hpp"

using namespace shmdata;

ShmdataWriter shmdata_make_writer(const char *path,
                                  size_t memsize,
                                  const char *type_descr,
                                  ShmdataCLogger log){
  return static_cast<void *>(new Writer(path,
                                        memsize,
                                        type_descr,
                                        static_cast<AbstractLogger *>(log)));
}

void shmdata_delete_writer(ShmdataWriter writer){
  delete static_cast<Writer *>(writer);
}

int shmdata_copy_to_shm(ShmdataWriter writer, void *data, size_t size){
  return static_cast<Writer *>(writer)->copy_to_shm(data, size);
}

