#include <cstdint>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

#include "../headers/DataBaseManager.h"
#include "../headers/converts.h"
#include "../headers/BinaryFileHandler.h"
#include "../headers/paths.h"

using namespace DataBaseManager;

void BinaryFileCHandler::createFiles_C(vector<pair<KeyType, countOperationFileB>> allData) {

    // crear los archivos C
    ofstream recordsFileC(pathRecordsFileC, ios::binary | ios::trunc);
    ofstream metadataFileC(pathMetadataFileC, ios::binary | ios::trunc);

    if (!recordsFileC || !metadataFileC) {
        throw runtime_error("Error al abrir los archivos");
    }

    for (const auto& [key, value]: allData){
        // Escribir type (1 byte)
        recordsFileC.write(reinterpret_cast<const char*>(&key.first), sizeof(uint8_t));

        // Obtener posición inicial del value (después del type)
        const uint64_t pos = recordsFileC.tellp();

        // Escribir value (UTF-32)
        recordsFileC.write(
            reinterpret_cast<const char*>(value.operation.value.data()),
            value.operation.value.size() * sizeof(char32_t)
        );

        // Escribir metadatos 
        metadataFileC.write(reinterpret_cast<const char*>(&value.operation.len), sizeof(uint64_t));
        metadataFileC.write(reinterpret_cast<const char*>(&value.amount), sizeof(uint64_t));
        metadataFileC.write(reinterpret_cast<const char*>(&pos), sizeof(uint64_t));


        // Debug
        // cout << "Type: " << static_cast<int>(key.first)
        //     << " - Value: " << utf32ToUtf8(value.operation.value)
        //     << " - idRecord: "<<value.operation.idRecord
        //     << " - len: "<<value.operation.len
        //     << " - Amount: " << value.amount
        //     << " - Pos: " << pos << endl;

    }

    recordsFileC.close();
    metadataFileC.close();

}


void BinaryFileCHandler::readTwoBinaryFles_C() {
    
    ifstream recordsFileC(pathRecordsFileC, ios::binary);
    ifstream metadataFileC(pathMetadataFileC, ios::binary);

    if (!metadataFileC.is_open() || !recordsFileC.is_open()) {
        cerr << "Error al abrir los archivos\n";
        return;
    }

    DataBaseManager::struct_metadataFileC metadata;
    while (metadataFileC.read(reinterpret_cast<char*>(&metadata), sizeof(DataBaseManager::struct_metadataFileC))) {
        // Leer type (1 byte antes de la posición del value)
        recordsFileC.seekg(metadata.pos - streamoff(1));
        uint8_t type;
        recordsFileC.read(reinterpret_cast<char*>(&type), sizeof(uint8_t));

        // Posicionarse al inicio del value
        recordsFileC.seekg(metadata.pos);

        // Leer value (UTF-32)
        vector<char32_t> value(metadata.len / sizeof(char32_t));
        recordsFileC.read(reinterpret_cast<char*>(value.data()), metadata.len);

        // Sin terminador nulo: construir el u32string desde iteradores.
        u32string valor(value.begin(), value.end());

        cout<< "Type: " << static_cast<int>(type)<<" - ";
        cout<<tokenParaMostrar(valor);
        cout<< " - len: "<<metadata.len 
            << " - amount: "<<metadata.amount
            << " - pos: "<<metadata.pos<<endl;
    }

    metadataFileC.close();
    recordsFileC.close();
}