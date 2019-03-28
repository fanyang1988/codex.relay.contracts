#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

#pragma once

using namespace eosio;

// now just for token map
CONTRACT siderelay : public contract {
public:
   using contract::contract;
   siderelay( name receiver, name code, datastream<const char*> ds )
      : contract(receiver, code, ds)
      , workergroups(receiver, receiver.value) {}

   // TODO Support diff token contracts

   constexpr static auto token_map_typ = "map.token"_n;

   // from side chain to relay
   ACTION in( uint64_t num,  capi_name to, const asset& quantity, const std::string& memo );

   // from relay chain to side
   ACTION out( capi_name committer, uint64_t num, capi_name to, name chain, name contract, const asset& quantity, const std::string& memo );

   // change transfers
   ACTION chworker( capi_name committer, capi_name chain, capi_name old, capi_name worker, uint64_t power, const permission_level& permission );

   // change worker in bios stage
   ACTION initworker( capi_name chain, capi_name worker_typ, capi_name worker, uint64_t power, const permission_level& permission );

   ACTION cleanworker( capi_name chain );

   TABLE workersgroup {
      public:
         name group_name;
         std::vector<name>             requested_names;
         std::vector<uint64_t>         requested_powers;
         std::vector<permission_level> requested_approvals;
         uint64_t power_sum = 0;

      private:
         int get_idx_by_name( capi_name worker ) const;

      public:
         name check_permission( capi_name worker ) const;
         void modify_worker( capi_name worker, uint64_t power, const permission_level& permission );
         void clear_workers();
         void del_worker( capi_name worker );
         bool is_confirm_ok( const std::vector<name>& confirmed ) const;

      public:
         uint64_t primary_key()const { return group_name.value; }
   };
   typedef eosio::multi_index< "workersgroup"_n, workersgroup > workersgroup_table;
   workersgroup_table workergroups;

   TABLE workstate {
      public:
         uint64_t    confirmed_num;
         name        type;

      public:
         uint64_t primary_key()const { return type.value; }
   };
   typedef eosio::multi_index< "workstates"_n, workstate > workstate_table;

   struct outaction_data {
      capi_name   to;
      name        chain;
      name        contract;
      asset       quantity;
      std::string memo;

      std::vector<name>  confirmed;

      outaction_data() = default;
      ~outaction_data() = default;
      outaction_data( const outaction_data& ) = default;

      outaction_data( capi_name to, const name& chain,
                      const name& contract, const asset& quantity,
                      const std::string& memo )
         : to(to)
         , chain(chain)
         , contract(contract)
         , quantity(quantity)
         , memo(memo)
      {}

      friend constexpr bool operator == ( const outaction_data& a, const outaction_data& b ) {
         return std::tie( a.to, a.chain, a.contract, a.quantity, a.memo )
             == std::tie( b.to, b.chain, b.contract, b.quantity, b.memo );
      }

      EOSLIB_SERIALIZE( outaction_data,
         (to)(chain)(contract)(quantity)(memo)(confirmed) )
   };

   TABLE outaction {
      public:
         outaction() = default;

         uint64_t                    num = 0;
         std::vector<outaction_data> actions;

         bool commit( capi_name committer,
                      const workersgroup& workers,
                      const outaction_data& act );

      public:
         uint64_t primary_key() const { return num; }
   };
   typedef eosio::multi_index< "outactions"_n, outaction > outaction_table;

public:
   void ontransfer( capi_name from, capi_name to, const asset& quantity, const std::string& memo );

private:
   void do_out( const name& to, const name& chain, const asset& quantity, const std::string& memo );

public:
   using in_action = action_wrapper<"in"_n, &siderelay::in>;
   using out_action = action_wrapper<"out"_n, &siderelay::out>;
};