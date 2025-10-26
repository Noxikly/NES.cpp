#pragma once
#include "mapper.hpp"
#include "mappers/mapper0.hpp"
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
            default:
                throw std::runtime_error("Маппер "+std::to_string(mapperNum)+" не поддерживается");
        }
    }
};
