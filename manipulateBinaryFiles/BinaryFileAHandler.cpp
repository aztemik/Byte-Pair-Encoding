#include <cstdint>
#include <exception>
#include <ostream>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <stdexcept>

#include "../headers/DataBaseManager.h"
#include "../headers/GeneralBinaryFileHandler.h"
#include "../headers/BinaryFileHandler.h"
#include "../headers/paths.h"
#include "../headers/converts.h"

using namespace std;

// Escribe un registro en los flujos ya abiertos y avanza el desplazamiento.
// Antes esta funcion abria el archivo cuatro veces por cada par (una para el
// tipo, otra para consultar la posicion al sistema de archivos, otra para el
// valor y otra para los metadatos). Con millones de pares eso son millones de
// aperturas; ahora los flujos se abren una sola vez en createFiles_A y la
// posicion se lleva en una variable.
void BinaryFileAHandler::insertRecordsToFileA(
    const u32string& value,
    ofstream& recordsFile,
    ofstream& metadataFile,
    uint64_t& offset
    ){

    const uint64_t len = value.size() * sizeof(char32_t);
    const DataBaseManager::ValueType type = DataBaseManager::ValueType::STRING_UTF32;

    // Escribir type (1 byte). El value empieza justo despues.
    recordsFile.write(reinterpret_cast<const char*>(&type), sizeof(uint8_t));
    const uint64_t pos = offset + 1;

    // Escribir value (UTF-32)
    recordsFile.write(reinterpret_cast<const char*>(value.data()), len);

    // Escribir metadatos: longitud y desplazamiento del value
    const DataBaseManager::struct_metadataFileA record = { len, pos };
    metadataFile.write(reinterpret_cast<const char*>(&record), sizeof(DataBaseManager::struct_metadataFileA));

    offset = pos + len;
}

int BinaryFileAHandler::createFiles_A (const vector<u32string>& onlyPares){

    // ios::trunc, no ios::app: el almacen A se reconstruye entero en cada
    // entrenamiento. Con append, entrenar dos veces acumulaba los pares de la
    // ejecucion anterior y falseaba todas las frecuencias.
    ofstream recordsFileA(pathRecordsFileA, ios::binary | ios::trunc);
    ofstream metadataFileA(pathMetadataFileA, ios::binary | ios::trunc);

    if (!recordsFileA || !metadataFileA){
        throw runtime_error("Error al abrir los archivos del almacen A");
    }

    uint64_t offset = 0;
    for (const auto& par : onlyPares){
        insertRecordsToFileA(par, recordsFileA, metadataFileA, offset);
    }

    recordsFileA.close();
    metadataFileA.close();

    return 0;
};


void BinaryFileAHandler::readTwoBinaryFiles_A(){

    ifstream recordsFileA(pathRecordsFileA, ios::binary);
    ifstream metadataFileA(pathMetadataFileA, ios::binary);

    if (!metadataFileA.is_open() || !recordsFileA.is_open()){
        cerr<<"Error al abrir los archivos\n";
        return;
    }

    DataBaseManager::struct_metadataFileA metadata;

    while(metadataFileA.read(reinterpret_cast<char*>(&metadata), sizeof(DataBaseManager::struct_metadataFileA))){
        // Leer type (1 byte antes de la posición del value)
        recordsFileA.seekg(metadata.pos - streamoff(1));
        uint8_t type;
        recordsFileA.read(reinterpret_cast<char*>(&type), sizeof(uint8_t));

        // Posicionarse al inicio del value
        recordsFileA.seekg(metadata.pos);

        // Leer value (UTF-32)
        vector<char32_t> value(metadata.len / sizeof(char32_t));
        recordsFileA.read(reinterpret_cast<char*>(value.data()), metadata.len);

        // Sin terminador nulo: construir el u32string desde iteradores.
        u32string valor(value.begin(), value.end());

        // Mostrar datos
        cout << "Type: " << static_cast<int>(type)<<" - Value: ";
        cout<<tokenParaMostrar(valor);
        cout<< " - len: "<<metadata.len
            << " - Pos: " << metadata.pos<<endl;
    }
    metadataFileA.close();
    recordsFileA.close();

    cout<<"\n";
}