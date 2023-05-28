#include "random.hpp"
#include "common.hpp"
#include "client.hpp"



namespace client {

   int run( const base::Parameters::tSptr parameters )
   {
      const int socket_family = common::string_to_socket_family(
            parameters->value_or< std::string >( "family", common::default_values::socket_family ).first
         );
      const int socket_type = common::string_to_socket_type(
            parameters->value_or< std::string >( "type", common::default_values::socket_type ).first
         );
      const int socket_protocole = parameters->value_or< int >(
            "protocole", common::default_values::socket_protocole
         ).first;

      int master_socket = socket( socket_family, socket_type, socket_protocole );
      if( -1 == master_socket )
      {
         int error = errno;
         printf( "'socket' error: %d\n", error );
         return error;
      }
      printf( "'socket' success: %d\n", master_socket );

      int result_connect = -1;
      if( AF_UNIX == socket_family )
      {
         const char* address = parameters->value_or< std::string >(
               "address", common::default_values::address_unix
            ).first.c_str( );

         struct sockaddr_un addr_un;
         memset( &addr_un, 0, sizeof( sockaddr_un ) );
         addr_un.sun_family = socket_family;
         strncpy( addr_un.sun_path, address, sizeof(addr_un.sun_path) - 1 );
         int addr_un_size = sizeof( addr_un.sun_family ) + strlen( addr_un.sun_path );

         result_connect = connect( master_socket, (struct sockaddr*) &addr_un, addr_un_size );
      }
      else if( AF_INET == socket_family )
      {
         const char* address_cstr = parameters->value_or< std::string >(
               "address", common::default_values::address_inet
            ).first.c_str( );
         in_addr address_buf;
         int result = inet_pton( AF_INET, address_cstr, (char*)&address_buf );
         if( 0 == result )
         {
            int error = errno;
            printf( "'inet_pton' error: %d\n", error );
            printf( "'address' is not in presentation format\n" );
            return error;
         }
         else if( 0 > result )
         {
            int error = errno;
            printf( "'inet_pton' error: %d\n", error );
            return error;
         }
         uint32_t address = address_buf.s_addr;
         // uint32_t address = inet_addr( address_cstr );
         // uint32_t address = htonl( INADDR_ANY );
         const int port = parameters->value_or< int >( "port", common::default_values::port ).first;

         struct sockaddr_in addr_in;
         memset( &addr_in, 0, sizeof( sockaddr_in ) );
         addr_in.sin_family = socket_family;
         addr_in.sin_addr.s_addr = address;
         addr_in.sin_port = htons( port );
         int addr_in_size = sizeof( addr_in );

         result_connect = connect( master_socket, (struct sockaddr*) &addr_in, addr_in_size );
      }
      else if( AF_VSOCK == socket_family )
      {
         const int address = parameters->value_or< int >( "address", common::default_values::address_vsock ).first;
         const int port = parameters->value_or< int >( "port", common::default_values::port ).first;

         struct sockaddr_vm addr_vm;
         memset( &addr_vm, 0, sizeof( sockaddr_vm ) );
         addr_vm.svm_family = socket_family;
         addr_vm.svm_cid = address;
         addr_vm.svm_port = port;
         int addr_vm_size = sizeof( addr_vm );

         result_connect = connect( master_socket, (sockaddr*)&addr_vm, addr_vm_size );
      }

      if( -1 == result_connect )
      {
         int error = errno;
         printf( "'connect' error: %d\n", error );
         return error;
      }
      printf( "'connect' success\n" );

      std::string send_message;
      std::string recv_message;
      const size_t buffer_size = common::default_values::message_max_size;
      char buffer[ buffer_size ];
      const std::string mode = parameters->value_or< std::string >( "mode", common::default_values::mode ).first;
      const int timeout = parameters->value_or< int >( "timeout", common::default_values::timeout ).first;
      const size_t message_length = parameters->value_or< int >( "length", common::default_values::message_length ).first;
      size_t message_size;
      while( true )
      {
         // Build message

         if( "count" == mode )
         {
            usleep( 1000 * timeout );
            static size_t count = 0;
            send_message = std::to_string( count++ );
            if( 100 == count )
               break;
         }
         else if( "message" == mode )
         {
            printf( "Enter a message:\n" );
            send_message.clear( );
            std::getline( std::cin, send_message );
            if( "exit" == send_message )
               break;
         }
         else if( "random" == mode )
         {
            usleep( 1000 * timeout );
            if( 0 == message_length )
               message_size = sot::random::number( 1, common::default_values::message_max_size - 1 );
            else if( common::default_values::message_max_size < message_length )
               message_size = common::default_values::message_max_size;
            send_message = sot::random::text( message_size );
         }

         // Send message to server
         ssize_t send_size = send( master_socket, send_message.c_str( ), send_message.length( ), 0 );
         if( -1 == send_size )
         {
            int error = errno;
            printf( "'send' error: %d\n", error );
            return error;
         }
         printf( "'send' success: %ld bytes / '%s'\n", send_size, send_message.c_str( ) );

         // Receive message from server
         ssize_t recv_size = recv( master_socket, &buffer, buffer_size - 1, 0 );
         if( 0 >= recv_size )
         {
            int error = errno;
            if( EAGAIN == error )
            {
               printf( "'recv' error: %d\n", error );
               continue;
            }
            else
            {
               printf( "server disconnected\n" );
               break;
            }
         }
         buffer[ recv_size ] = 0;
         printf( "'recv' success: %ld bytes / '%s'\n", recv_size, buffer );

         // Compare messages
         if( send_message == buffer )
         {
            printf( "callback ok\n" );
         }
         else
         {
            printf( "callback nok\n" );
         }
      }

      close( master_socket );

      return 0;
   }

}
