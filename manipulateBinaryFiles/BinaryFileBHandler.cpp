#include <algorithm>
#include <cstdint>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <vector>

#include "../headers/converts.h"
#include "../headers/DataBaseManager.h"
#include "../headers/BinaryFileHandler.h"
#include "../headers/paths.h"

using namespace std;

vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> BinaryFileBHandler::loadTwoFiles_A(){
    
    map<DataBaseManager::KeyType, DataBaseManager::countOperationFileB> mapSorted;

    // read records
    std::ifstream recordsFileA(pathRecordsFileA, std::ios::binary);
    if (!recordsFileA.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de metadatos: " << pathRecordsFileA << std::endl;
    }   

    // read metadata
    std::ifstream metadataFileA(pathMetadataFileA, std::ios::binary);
    if (!metadataFileA.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de datos: " << pathMetadataFileA << std::endl;
        recordsFileA.close();
    }
    
    DataBaseManager::struct_metadataFileA recordsMetadataFileA;
    // int recordCount = 0;
    
    while (metadataFileA.read(reinterpret_cast<char*>(&recordsMetadataFileA), sizeof(DataBaseManager::struct_metadataFileA))) {
        
        // Leer el tipo (retroceder 1 byte desde pos)
        recordsFileA.seekg(recordsMetadataFileA.pos - static_cast<std::streamoff>(1));
        uint8_t typeValue;
        if (!recordsFileA.read(reinterpret_cast<char*>(&typeValue), sizeof(uint8_t))) { // here read type
            std::cerr << "Error al leer el tipo de dato\n";
            break;
        }
        // std::cout << "Tipo leído (hex): " << std::hex << static_cast<int>(typeValue) << std::dec << "\n";
        
        // leer value
        // Crea un vector de caracteres UTF-32 con tamaño suficiente para almacenar los datos
        std::vector<char32_t> buffer(recordsMetadataFileA.len / sizeof(char32_t)); // calcula el espacio para almacenar los caracteres de 4 bytes
        // si len = 12 bytes y sizeof(char32_t) = 4, el buffer tendrá espacio para 3 caracteres
        recordsFileA.read(reinterpret_cast<char*>(buffer.data()), recordsMetadataFileA.len);
        // buffer.data() devuelve un puntero al array interno del vector

        // Convierte el buffer a un string UTF-32
        std::u32string value(buffer.begin(), buffer.end());
        // buffer.begin() y buffer.end() son iteradores que apuntan al inicio y fin del buffer

        // valores reales para el registro
        // std::cout << "\nRegistro #" << ++recordCount << ":\n"; // este no se almacena en el registro
        // std::cout << "Tipo leído (hex): " << std::hex << static_cast<int>(typeValue) << std::dec << "\n";
        // param_u32string_see(value);
        // std::cout << "ID: " << recordsMetadataFileA.idRecord << "\n";
        // std::cout << "Longitud: " << recordsMetadataFileA.len << " bits\n";
        // std::cout << "Posición: " << recordsMetadataFileA.pos << "\n";

        DataBaseManager::operationsFileB operation = {
            typeValue,
            value, 
            recordsMetadataFileA.len
        };

        
        DataBaseManager::KeyType key = {typeValue, value}; // crear clave para conteo
        // cout<<"Key generado como identificador de conteo (convertToUtf8): "<<keyToString(key)<<endl;

        // 2. Actualizar conteo de ocurrencias junto con el registro operation
        // Si la key ya existe, incrementa el contador
        // Si no existe, la crea con contador 1
        try{
            auto& entry = mapSorted[key];
            entry.operation = operation;    // Actualiza la operación
            entry.amount++;                 // Incrementa el contador
        } catch (const exception& e) {
            cout<<"Error: "<<e.what()<<endl;
        }
    
        // std::cout << "------------------------\n";
    
    }

    
    metadataFileA.close();
    recordsFileA.close();

    // 2. Transferencia a vector para ordenamiento
    vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> ordenado(mapSorted.begin(), mapSorted.end());
    
    // 3. Ordenamiento
    sort(ordenado.begin(), ordenado.end(),
    [](const auto& a, const auto& b) {
        return a.second.amount > b.second.amount;  // mayor a menor
    });

    return ordenado;
};

void BinaryFileBHandler::mostrarVector(
    vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> VectorOrdenadoYAllData
) {

    for (const auto& [key, value]: VectorOrdenadoYAllData){
        cout<<"Type: "<<key.first;
        utf32ToUtf8(value.operation.value);
        cout<<" - len: "<<value.operation.len
            <<" - Amount: "<<value.amount<<endl;;
    }


}

void BinaryFileBHandler::createFiles_B (
    const vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>>& allDataForFiles_B
) {

    ofstream recordsFileB(pathRecordsFileB, ios::binary | ios::trunc);
    ofstream metadataFileB(pathMetadataFileB, ios::binary | ios::trunc);

    if (!recordsFileB || !metadataFileB) {
        throw runtime_error("Error al abrir los archivos");
    }

    for (const auto& [key, value] : allDataForFiles_B) {
        // Escribir type (1 byte)
        recordsFileB.write(reinterpret_cast<const char*>(&key.first), sizeof(uint8_t));

        // Obtener posición inicial del value (después del type)
        const uint64_t pos = recordsFileB.tellp();

        // Escribir value (UTF-32)
        recordsFileB.write(
            reinterpret_cast<const char*>(value.operation.value.data()),
            value.operation.value.size() * sizeof(char32_t)
        );

        // Escribir amount (8 bytes)
        recordsFileB.write(reinterpret_cast<const char*>(&value.amount), sizeof(uint64_t));

        // Escribir metadatos
        metadataFileB.write(reinterpret_cast<const char*>(&value.operation.len), sizeof(uint64_t));
        metadataFileB.write(reinterpret_cast<const char*>(&pos), sizeof(uint64_t));

        // Debug
        // cout << "Type: " << static_cast<int>(key.first)
        //      << " - Value: " << utf32ToUtf8(value.operation.value)
        //      << " - Amount: " << value.amount
        //      << " - len: "<<value.operation.len
        //      << " - Pos: " << pos << endl;
    }

    recordsFileB.close();
    metadataFileB.close();
};


void BinaryFileBHandler::readTwoBinaryFiles_B() {
    ifstream metadataFileB(pathMetadataFileB, ios::binary);
    ifstream recordsFileB(pathRecordsFileB, ios::binary);

    if (!metadataFileB.is_open() || !recordsFileB.is_open()) {
        cerr << "Error al abrir los archivos\n";
        return;
    }

    DataBaseManager::metadataFileB metadata;
    while (metadataFileB.read(reinterpret_cast<char*>(&metadata), sizeof(DataBaseManager::metadataFileB))) {
        // Leer type (1 byte antes de la posición del value)
        recordsFileB.seekg(metadata.pos - streamoff(1));
        uint8_t type;
        recordsFileB.read(reinterpret_cast<char*>(&type), sizeof(uint8_t));

        // Posicionarse al inicio del value
        recordsFileB.seekg(metadata.pos);

        // Leer value (UTF-32)
        vector<char32_t> value(metadata.len / sizeof(char32_t));
        recordsFileB.read(reinterpret_cast<char*>(value.data()), metadata.len);

        // Leer amount
        uint64_t amount;
        recordsFileB.read(reinterpret_cast<char*>(&amount), sizeof(uint64_t));

        // Sin terminador nulo: construir el u32string desde iteradores.
        u32string valor(value.begin(), value.end());

        // Mostrar datos
        cout << "Type: " << static_cast<int>(type)<<" - ";
        cout<<tokenParaMostrar(valor);
        cout<< " - len: "<<metadata.len
            << " - Pos: " << metadata.pos
            << " - amount: "<<amount<<endl;
    }
    metadataFileB.close();
    recordsFileB.close();
}