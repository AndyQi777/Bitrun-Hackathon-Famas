#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/system.h>
#include <graphenelib/types.h>

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <cmath>
#include <math.h>
#include <vector>
#include <ctime>

using namespace graphene;

class cat : public contract
{
  public:
    cat(uint64_t account_id)
        : contract(account_id)
        , accounts(_self, _self)
        , packets(_self, _self)
    {
    }

    //@abi action
    //@abi payable
    void deposit()
    {
        // 通过get_action_asset_amount与get_action_asset_id获取调用action时附加的资产id和资产数量
        int64_t asset_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        contract_asset amount{asset_amount, asset_id};

        // 获取调用者id
        uint64_t owner = get_trx_sender();
        auto it = accounts.find(owner);

        //位注册用户免费赠送一只电子猫，共五种不同属性1-5，随机抽取
        int InitialCat = 0;
        int64_t block_num = get_head_block_num();
        std::string random_str = std::to_string(asset_id) + std::to_string(block_num);
        checksum160 sum160;
        ripemd160(const_cast<char *>(random_str.c_str()), random_str.length(), &sum160);
        InitialCat = (int(sum160.hash)) % 5;

        // 三种猫粮，level1-3由普通到高级
        int catfoodlevel1 = 1;
        int catfoodlevel2 = 3;
        int catfoodlevel3 = 6;
        // 两种特殊道具
        int specialkind1 = 6.25;
        int specialkind2 = 6.75;
        
        //循环n次，n为玩家所拥有的探寻猫粮的次数(虚拟货币数)
        int n = amount.amount;

        //自此之后，amount.amount表示玩家所拥有的猫粮数
        amount.amount = 0;
        amount.asset_id = 0;
        for (int j = 0; j <= n; j++){
            // 获得0-10（不包含）的随机数i
            int i = 0;
            int64_t block_num = get_head_block_num();
            std::string random_str = std::to_string(n) + std::to_string(block_num);
            checksum160 sum160;
            ripemd160(const_cast<char *>(random_str.c_str()), random_str.length(), &sum160);
            i = (int(sum160.hash)) % 10;

            // 判断所处位置有没有猫粮
            if (i <= catfoodlevel1){
                amount.amount += 3;
                amount.asset_id += 3;
            }
            else if (i <= catfoodlevel2){
                amount.amount += 2;
                amount.asset_id +=2;
            }
            else if (i <= catfoodlevel3){
                amount.amount += 1;
                amount.asset_id +=1;
            }
            else if (i <= specialkind1){
                //licking, dancing, 特殊互动
            }
            else if (i <= specialkind2){
                //smiling, swinging，特殊互动
            }
            else{
                amount.amount += 0;
                amount.asset_id += 0;
            };
        };
        
        //如果调用者尚未存储过资产，则为其创建table项，主键为用户instance_id
        //chances表示玩家得到的探索所在处猫粮的机会
        if (it == accounts.end()) {
            accounts.emplace(owner, [&](auto &o) {
                o.owner = owner;
                o.chances.emplace_back(amount);
            });
        } else {
            uint16_t asset_index = std::distance(it->chances.begin(), 
                                                 find_if(it->chances.begin(), it->chances.end(), [&](const auto &a) { return a.asset_id == asset_id; }));
            if (asset_index < it->chances.size()) {
                accounts.modify(it, 0, [&](auto &o) { o.chances[asset_index] += amount; });
            } else {
                accounts.modify(it, 0, [&](auto &o) { o.chances.emplace_back(amount); });
            }
        }
        int64_t &p = amount.amount;
        uint64_t &q = amount.asset_id;
    }

    //@abi action
    //@abi payable
    void feed(std::string pubkey,contract_asset amount)
    {
        //查验公钥
        std::string prefix("GXC");
        const size_t prefix_len = prefix.size();
        graphene_assert(pubkey.size() > prefix_len, "invalid public key");
        graphene_assert(pubkey.substr(0, prefix_len) == prefix, "invalid public key");

        int64_t total_amount = get_action_asset_amount();
        uint64_t asset_id = get_action_asset_id();
        uint64_t owner = get_trx_sender();
        //键入投喂的猫粮数
        packets.emplace(owner, [&](auto &o) {
            o.owner = owner;
            o.pub_key = pubkey;
            o.total_amount = contract_asset{total_amount, asset_id};
            if (amount.amount >= total_amount){
                int64_t difference = amount.amount - total_amount;
                o.chances.emplace_back(difference);
            }else{
                graphene_assert(amount.amount < total_amount, "invalid");
            }
                
        });

    }
  private:
    //@abi table account i64
    struct account {
        uint64_t owner;
        std::vector<contract_asset> chances;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(chances))
    };

    typedef graphene::multi_index<N(account), account> account_index;

    account_index accounts;

        struct packet {
        uint64_t                owner;
        std::string             pub_key;
        contract_asset          total_amount;
        uint32_t                number;
        vector<int64_t>         chances;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(packet, (owner)(pub_key)(total_amount)(number)(chances))
    };
    typedef graphene::multi_index<N(packet), packet> packet_index;
    packet_index            packets;
};

GRAPHENE_ABI(cat, (deposit)(feed))
