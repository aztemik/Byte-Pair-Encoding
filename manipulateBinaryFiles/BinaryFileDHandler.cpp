#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../headers/BinaryFileHandler.h"
#include "../headers/DataBaseManager.h"
#include "../headers/converts.h"
#include "../headers/paths.h"

using namespace std;

// Almacen D: las reglas de fusion, en el orden en que el entrenamiento las
// aprendio. Ese orden es parte del algoritmo, no un detalle: al tokenizar hay
// que aplicarlas en la misma secuencia o el resultado no coincide con el
// entrenamiento. Por eso el "id" de una regla es su posicion en el archivo,
// igual que el id de un token en el almacen C es su posicion.
//
// Sigue el mismo patron que A, B y C: un archivo de registros con los valores
// en crudo y otro de metadatos de tamano fijo, para llegar a cualquier regla
// por acceso directo sin recorrer el archivo.
void BinaryFileDHandler::createFiles_D(const vector<DataBaseManager::mergeRule>& reglas) {

    ofstream recordsFileD(pathRecordsFileD, ios::binary | ios::trunc);
    ofstream metadataFileD(pathMetadataFileD, ios::binary | ios::trunc);

    if (!recordsFileD || !metadataFileD) {
        throw runtime_error("Error al abrir los archivos del almacen D");
    }

    const DataBaseManager::ValueType type = DataBaseManager::ValueType::STRING_UTF32;
    uint64_t offset = 0;

    for (const auto& regla : reglas) {
        const uint64_t len1 = regla.par1.size() * sizeof(char32_t);
        const uint64_t len2 = regla.par2.size() * sizeof(char32_t);

        // Escribir type (1 byte). par1 empieza justo despues.
        recordsFileD.write(reinterpret_cast<const char*>(&type), sizeof(uint8_t));
        const uint64_t pos = offset + 1;

        recordsFileD.write(reinterpret_cast<const char*>(regla.par1.data()), len1);
        recordsFileD.write(reinterpret_cast<const char*>(regla.par2.data()), len2);

        const DataBaseManager::struct_metadataFileD record = { len1, len2, pos };
        metadataFileD.write(reinterpret_cast<const char*>(&record),
                            sizeof(DataBaseManager::struct_metadataFileD));

        offset = pos + len1 + len2;
    }

    recordsFileD.close();
    metadataFileD.close();
}


vector<DataBaseManager::mergeRule> BinaryFileDHandler::loadMergeRules() {

    vector<DataBaseManager::mergeRule> reglas;

    ifstream recordsFileD(pathRecordsFileD, ios::binary);
    ifstream metadataFileD(pathMetadataFileD, ios::binary);

    if (!recordsFileD.is_open() || !metadataFileD.is_open()) {
        cerr << "Error al abrir los archivos del almacen D\n";
        return reglas;
    }

    DataBaseManager::struct_metadataFileD metadata;
    while (metadataFileD.read(reinterpret_cast<char*>(&metadata),
                              sizeof(DataBaseManager::struct_metadataFileD))) {

        recordsFileD.seekg(metadata.pos);

        vector<char32_t> buffer1(metadata.len1 / sizeof(char32_t));
        recordsFileD.read(reinterpret_cast<char*>(buffer1.data()), metadata.len1);

        vector<char32_t> buffer2(metadata.len2 / sizeof(char32_t));
        recordsFileD.read(reinterpret_cast<char*>(buffer2.data()), metadata.len2);

        // Desde iteradores: los buffers no llevan terminador nulo.
        DataBaseManager::mergeRule regla{
            u32string(buffer1.begin(), buffer1.end()),
            u32string(buffer2.begin(), buffer2.end())
        };
        reglas.push_back(regla);
    }

    metadataFileD.close();
    recordsFileD.close();

    return reglas;
}


void BinaryFileDHandler::readTwoBinaryFiles_D() {

    const vector<DataBaseManager::mergeRule> reglas = loadMergeRules();

    cout << "Reglas de fusion aprendidas: " << reglas.size() << "\n";
    for (size_t i = 0; i < reglas.size(); ++i) {
        cout << "#" << i
             << "  (" << tokenParaMostrar(reglas[i].par1)
             << " + " << tokenParaMostrar(reglas[i].par2)
             << ")  ->  " << tokenParaMostrar(reglas[i].resultado()) << "\n";
    }
    cout << endl;
}
