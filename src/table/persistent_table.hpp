/*
 * Copyright (c) 2014-17 The PipeFabric team,
 *                       All Rights Reserved.
 *
 * This file is part of the PipeFabric package.
 *
 * PipeFabric is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License (GPL) as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.
 * If not you can find the GPL at http://www.gnu.org/copyleft/gpl.html
 */

#ifndef persistent_table_hpp_
#define persistent_table_hpp_

#include <type_traits>

#include "table/TableInfo.hpp"
#include "core/Tuple.hpp"

#include "nvml/include/libpmemobj++/p.hpp"
#include "nvml/include/libpmemobj++/persistent_ptr.hpp"
#include "nvml/include/libpmemobj++/pool.hpp"
#include "nvml/include/libpmemobj++/make_persistent.hpp"
#include "nvml/include/libpmemobj++/transaction.hpp"
#include "nvml/include/libpmemobj++/utils.hpp"

using nvml::obj::pool;
using nvml::obj::pool_by_vptr;
using nvml::obj::pool_base;
using nvml::obj::p;
using nvml::obj::persistent_ptr;
using nvml::obj::make_persistent;
using nvml::obj::delete_persistent;
using nvml::obj::transaction;

namespace pfabric
{

constexpr unsigned short blockSize = 1 << 15; // 32KB

template <typename T, typename K>
class persistent_table
{
	//static_assert(std::is_base_of<pfabric::Tuple<typename... Types>, T>::value, "Records must be Tuple");
 
	public:
    typedef persistent_ptr<T> RecordType;
    typedef K KeyType; // BDCC Key?

    /* PAX oriented data blocks - structure of array - 32KB:
     * <ddc_range><ddc_cnt><sma_offset_0><data_offset_0> ...<sma_offset_n><data_offset_n>
     * <sma_min_0><sma_max_0><data_vector_0> ... <sma_min_n><sma_max_n><data_vector_n>
     *  0 ddc_range     -> long (x2) - 8 Byte
     *  8 ddc_cnt       -> long - 4 Byte
		 * 12 free_space    -> unsigned short 
     * 14 sma_offset_x  -> unsigned short - 2 Byte (depends on block size)
     * 16 data_offset_x -> unsigned short 
     *  . sma_min_x     -> size of attributes data type
     *  . sma_max_x     -> size of attributes data type
     *  . data_vector   -> size of attributes data type * ddc_cnt
     *
     * TODO: Add compression later
     */
    typedef typename std::array<unsigned char, blockSize> DDC_Block;

    persistent_table()
    {
        auto pop = pool_by_vptr(this);
        transaction::exec_tx(pop, [&] { init(TableInfo()); });
    }

    persistent_table(const TableInfo& _tInfo)
    {
        auto pop = pool_by_vptr(this);
        transaction::exec_tx(pop, [&] { init(_tInfo); });
    }

    void init(const TableInfo& _tInfo)
    {
        this->root = make_persistent<struct root>();
        this->root->tInfo = make_persistent<TableInfo>(_tInfo);
        this->root->block_list = make_persistent<struct ddc_block>();
        DDC_Block first = initBlock();
        this->root->block_list->block = make_persistent<DDC_Block>(first);
    }

    DDC_Block initBlock()
    {
        auto b = DDC_Block{
            0x00, 0x00, 0x00, 0x00,  // BDCC Range
            0xFF, 0xFF, 0xFF, 0xFF,  // BDCC Range
            0x00, 0x00, 0x00, 0x00}; // BDCC Count

				auto colCnt = 0; 
				for(auto& c: *this->root->tInfo)
					colCnt++;

				uint16_t header_size = 14 + colCnt * 4; // Fixed fields + attribute offsets
				uint16_t body_size = blockSize - header_size;

				auto minipage_size = body_size / colCnt;
				
				// Set Free Space field
        std::copy(static_cast<const unsigned char*>(static_cast<const void*>(&body_size)),
									static_cast<const unsigned char*>(static_cast<const void*>(&body_size)) + 2,
									b.begin() + 12);
				
				// Set Offsets
				unsigned i = 0;
				for(auto& c: *this->root->tInfo)
				{
					uint16_t sma_offset;
					uint16_t data_offset;
					switch(c.mColType)
					{
						case ColumnInfo::Int_Type :
						{
							sma_offset = header_size + minipage_size * i;
							data_offset = header_size + 8 + minipage_size * i;
						}
						break;
						case ColumnInfo::Double_Type :
						{
							sma_offset = header_size + minipage_size * i;
							data_offset = header_size + 16 + minipage_size * i;
						}
						break;
						case ColumnInfo::String_Type :
						{
							//TODO: What to do with variable length aggregates?
							sma_offset = header_size + minipage_size * i; 
							data_offset = header_size + minipage_size * i;
						}
						break;
						default :
						{
							throw TableException("unsupported column type\n");
						}
						break;
					}

        	std::copy(static_cast<const unsigned char*>(static_cast<const void*>(&sma_offset)),
										static_cast<const unsigned char*>(static_cast<const void*>(&sma_offset)) + 2,
										b.begin() + (14 + i * 4));
        	std::copy(static_cast<const unsigned char*>(static_cast<const void*>(&data_offset)),
										static_cast<const unsigned char*>(static_cast<const void*>(&data_offset)) + 2,
										b.begin() + (16 + i * 4));
					++i;
				}
				

        return b;
    }

    // Append only
    int insert(T rec, KeyType key)
    {
        auto pop = pool_by_vptr(this);
        auto dest_block = root->block_list;

        do
        {
            // Search correct block
            // Check for enough space
            //   yes, but not in minipage -> rearrange (average)
            //   no -> split block
            // Insert (For each attribute increase data vector and add value)
            // adapt SMAs and count  (How to cheaply increment)

            /*
						uint32_t key1 = (dest_block->block->at(0) << 24) |
                            (dest_block->block->at(1) << 16) |
                            (dest_block->block->at(2) << 8) |
                            (dest_block->block->at(3));
            uint32_t key2 = (dest_block->block->at(4) << 24) |
                            (dest_block->block->at(5) << 16) |
                            (dest_block->block->at(6) << 8) |
                            (dest_block->block->at(7));
            uint32_t cnt = (dest_block->block->at(11) << 24) |
                           (dest_block->block->at(10) << 16) |
                           (dest_block->block->at(9) << 8) |
                           (dest_block->block->at(8));
            uint16_t space = (dest_block->block->at(13) << 8) |
                             (dest_block->block->at(12));

            std::cout << "bdcc_range_min: " << key1 << std::endl;
            std::cout << "bdcc_range_max: " << key2 << std::endl;
            std::cout << "bdcc_count: " << cnt << std::endl;
            std::cout << "Free Space: " << space << std::endl;
						*/
            /*
						for (auto& b: *dest_block->block)
							std::cout << static_cast<unsigned int>(b) << " ";
						std::cout << std::endl;
						*/
						
            auto cur_block = dest_block->next;
            dest_block = cur_block;
        } while (dest_block != nullptr);

				
        return -1;
    }

    // TODO: For later
    int update(RecordType rec, KeyType key)
    {
        return 0;
    }

    int deleteByKey(KeyType key)
    {
        return 0;
    }

    RecordType getByKey(KeyType key)
    {
    }

    struct ddc_block
    {
        ddc_block() : next(nullptr), block(nullptr) {}
        ddc_block(DDC_Block _block) : next(nullptr), block(_block) {}

        persistent_ptr<struct ddc_block> next;
        persistent_ptr<DDC_Block> block;

        void clear()
        {
            if (next)
            {
                delete_persistent<struct ddc_block>(next);
                next = nullptr;
            }
            if (block)
            {
                delete_persistent<DDC_Block>(block);
                block = nullptr;
            }
        }
    };

    struct root
    {
        persistent_ptr<struct ddc_block> block_list;
        persistent_ptr<const TableInfo> tInfo;
        //persistent_ptr<std::string> tName;
    };

    persistent_ptr<struct root> root;

    void insert_block()
    {
    }
};

} /* namespace pfabric */

#endif /* persistent_table_hpp_ */
