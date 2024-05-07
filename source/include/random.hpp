// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2024, EPAM Systems



#pragma once

#include <cstdlib>
#include <string>



namespace sot::random {

   void init( );
   std::string text( size_t length );
   size_t number( size_t begin, size_t end );

}
