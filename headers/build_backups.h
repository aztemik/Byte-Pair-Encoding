#ifndef BUILDBACKUPS_H
#define BUILDBACKUPS_H

#include <exception>
#include <execution>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "paths.h"
class BuildBackups{
    private: 

    public:
    // Función para obtener fecha y hora actual en formato YYYYMMDD_HHMMSS
    static std::string obtenerFechaHoraPrecisa() {
        auto ahora = std::chrono::system_clock::now();
        auto tiempo = std::chrono::system_clock::to_time_t(ahora);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    ahora.time_since_epoch()) % 1000;
        
        std::tm tm_ahora = *std::localtime(&tiempo);
        
        std::stringstream ss;
        ss << std::put_time(&tm_ahora, "%Y%m%d_%H%M%S")
           << "_" << std::setfill('0') << std::setw(3) << ms.count();
        
        return ss.str();
    }

    // Función para crear backups con timestamp
    static bool hacerCopiaSeguridad(const std::string& pathOriginal, const std::string& whatFile) {
        std::string fechaHora = obtenerFechaHoraPrecisa();
        std::string archivoBackup = saveAnyChange + whatFile + "change_" + fechaHora + ".bin";
        
        std::ifstream origen(pathOriginal, std::ios::binary);
        std::ofstream backup(archivoBackup, std::ios::binary);
        
        if (!origen || !backup) return false;
        try{
            backup << origen.rdbuf();
        } catch (const std::exception& e){
            std::cout<<"Error: "<<e.what()<<std::endl;
        } 
        
        return origen.eof() && backup.good();
    };


};



#endif /* end of BUILDBACKUPS_H */