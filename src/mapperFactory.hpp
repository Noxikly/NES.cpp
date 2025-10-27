#pragma once
#include "mapper.hpp"
#include "mappers/mapper0.hpp"
#include "mappers/mapper1.hpp"
#include "mappers/mapper2.hpp"
#include "mappers/mapper3.hpp"
#include "mappers/mapper4.hpp"
#include "cartridge.hpp"
#include <memory>
#include <stdexcept>



class MapperFactory {
public:
    static auto createMapper(Cartridge& cart) -> std::unique_ptr<Mapper> {
        u8 mapperNum = cart.getMapperNumber();

        switch (mapperNum) {
            case 0:  /* NROM */
                return std::make_unique<Mapper0>(cart);
            case 1:  /* MMC1 */
                return std::make_unique<Mapper1>(cart);
            case 2:  /* UxROM */
                return std::make_unique<Mapper2>(cart);
            case 3:  /* CNROM */
                return std::make_unique<Mapper3>(cart);
            case 4:  /* MMC3 */
                return std::make_unique<Mapper4>(cart);

            default:
                throw std::runtime_error("Маппер "+std::to_string(mapperNum)+" не поддерживается");
        }
    }
};
