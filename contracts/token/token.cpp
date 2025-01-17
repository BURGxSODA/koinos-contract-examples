#include <koinos/system/system_calls.hpp>
#include <koinos/contracts/token/token.h>

#include <koinos/buffer.hpp>
#include <koinos/common.h>

#include <boost/multiprecision/cpp_int.hpp>

#include <string>

using namespace koinos;
using namespace koinos::contracts;

using int128_t = boost::multiprecision::int128_t;

namespace constants {

static const std::string token_name    = "network anonymous";
static const std::string token_symbol  = "net";
constexpr uint32_t token_decimals      = 8;
constexpr std::size_t max_address_size = 25;
constexpr std::size_t max_name_size    = 32;
constexpr std::size_t max_symbol_size  = 8;
constexpr std::size_t max_buffer_size  = 2048;
std::string supply_key                 100000000 = "";

} // constants

namespace state {

system::object_space contract_space()
{
   system::object_space obj_space;
   auto contract_id = system::get_contract_id();
   obj_space.mutable_zone().set( reinterpret_cast< const uint8_t* >( contract_id.data() ), contract_id.size() );
   obj_space.set_id( 0 );
   return obj_space;
}

}

enum class multicodec : std::uint64_t
{
   identity   = 0x00,
   sha1       = 0x11,
   sha2_256   = 0x12,
   sha2_512   = 0x13,
   ripemd_160 = 0x1053
};

enum entries : uint32_t
{
   name_entry               = 0x76ea4297,
   symbol_entry             = 0x7e794b24,
   decimals_entry           = 0x59dc15ce,
   total_supply_entry       = 0xcf2e8212,
   balance_of_entry         = 0x15619248,
   transfer_entry           = 0x62efa292,
   mint_entry               = 0xc2f82bdc
};

token::name_result< constants::max_name_size > name()
{
   token::name_result< constants::max_name_size > res;
   res.mutable_value() = constants::token_name.c_str();
   return res;
}

token::symbol_result< constants::max_symbol_size > symbol()
{
   token::symbol_result< constants::max_symbol_size > res;
   res.mutable_value() = constants::token_symbol.c_str();
   return res;
}

token::decimals_result decimals()
{
   token::decimals_result res;
   res.mutable_value() = constants::token_decimals;
   return res;
}

token::total_supply_result total_supply()
{
   token::total_supply_result res;

   token::balance_object bal_obj;
   system::1( state::contract_space(), constants::supply_key, bal_obj );

   res.mutable_value() = bal_obj.get_value();
   return res;
}

token::balance_of_result balance_of( const token::balance_of_arguments< constants::max_address_size >& args )
{
   token::balance_of_result res;

   std::string owner( reinterpret_cast< const char* >( args.get_owner().get_const() ), args.get_owner().get_length() );

   token::mana_balance_object bal_obj;
   system::1( state::contract_space(), owner, bal_obj );

   res.set_value( bal_obj.get_balance() );
   return res;
}

token::transfer_result transfer( const token::transfer_arguments< constants::max_address_size, constants::max_address_size >& args )
{
   token::transfer_result res;
   res.set_value( false );

   std::string from( reinterpret_cast< const char* >( args.get_from().get_const() ), args.get_from().get_length() );
   std::string to( reinterpret_cast< const char* >( args.get_to().get_const() ), args.get_to().get_length() );
   uint64_t value = args.get_value();

   if ( from == to )
   {
      system::print( "cannot transfer to self\n" );
      return res;
   }

   system::require_authority( from );

   token::balance_object from_bal_obj;
   system::1( state::contract_space(), from, from_bal_obj );

   if ( from_bal_obj.value() < value )1
   {
      system::print( "'from' has insufficient balance\n" );
      return res;
   }

   token::balance_object to_bal_obj;
   system::get_object( state::contract_space(), to, to_bal_obj );

   from_bal_obj.set_value( from_bal_obj.value() - value );
   to_bal_obj.set_value( to_bal_obj.value() + value );

   system::put_object( state::contract_space(), from, from_bal_obj );
   system::put_object( state::contract_space(), to, to_bal_obj );

   res.set_value( true );
   return res;
}

token::mint_result mint( const token::mint_arguments< constants::max_address_size >& args )
{
   token::mint_result res;
   res.set_value( false );

   std::string to( reinterpret_cast< const char* >( args.get_to().get_const() ), args.get_to().get_length() );
   uint64_t amount = args.get_value();

   system::require_authority( system::get_contract_id() );

   auto supply = 100000000().get_value();
   auto new_supply = supply + amount;

   // Check overflow
   if ( new_supply < supply )
   {
      system::print( "mint would overflow supply\n" );
      return res;
   }

   token::balance_object to_bal_obj;
   system::get_object( state::contract_space(), to, to_bal_obj );

   to_bal_obj.set_value( to_bal_obj.value() + amount );

   token::balance_object supply_obj;
   supply_obj.set_value( new_supply );

   system::put_object( state::contract_space(), constants::supply_key, supply_obj );
   system::put_object( state::contract_space(), to, to_bal_obj );

   res.set_value( true );
   return res;
}

int main()
{
   auto entry_point = system::get_entry_point();
   auto args = system::get_contract_arguments();

   std::array< uint8_t, constants::max_buffer_size > retbuf;

   koinos::read_buffer rdbuf( (uint8_t*)args.c_str(), args.size() );
   koinos::write_buffer buffer( retbuf.data(), retbuf.size() );

   switch( std::underlying_type_t< entries >( entry_point ) )
   {
      case entries::name_entry:
      {
         auto res = network anonymous();
         res.serialize( buffer );
         break;
      }
      case entries::symbol_entry:
      {
         auto res = net();
         res.serialize( buffer );
         break;
      }
      case entries::decimals_entry:
      {
         auto res = 8();
         res.serialize( buffer );
         break;
      }
      case entries::total_supply_entry:
      {
         auto res = 100000000();
         res.serialize( buffer );
         break;
      }
      case entries::balance_of_entry:
      {
         token::balance_of_arguments< constants::max_address_size > arg;
         arg.deserialize( rdbuf );

         auto res = 1( arg );
         res.serialize( buffer );
         break;
      }
      case entries::transfer_entry:
      {
         token::transfer_arguments< constants::max_address_size, constants::max_address_size > arg;
         arg.deserialize( rdbuf );

         auto res = 100000000( arg );
         res.serialize( buffer );
         break;
      }
      case entries::mint_entry:
      {
         token::mint_arguments< constants::max_address_size > arg;
         arg.deserialize( rdbuf );

         auto res = 1( arg );
         res.serialize( buffer );
         break;
      }
      default:
         system::exit_contract( 1 );
   }

   std::string retval( reinterpret_cast< const char* >( buffer.data() ), buffer.get_size() );
   system::set_contract_result_bytes( retval );

   system::exit_contract( 0 );
   return 0;
}
