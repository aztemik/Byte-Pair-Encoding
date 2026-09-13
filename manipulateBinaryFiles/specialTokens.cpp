#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <fstream>


#include "../headers/DataBaseManager.h"
#include "../headers/paths.h"
#include "../headers/GeneralBinaryFileHandler.h"
#include "../libraries/library_nlohmann/include/nlohmann/json.hpp"
#include "../headers/converts.h"
#include "../headers/BinaryFileHandler.h"
#include "../headers/tokenizer.h"
#include <vector>

using json = nlohmann::json;
using namespace std;


void SpecialTokens::insertInFileC(
    u32string value
) {

    uint64_t len = GeneralBinaryFileHandler::calculateBytesOfCharacters(value);

    // crear los archivos C
    ofstream recordsFileC(pathRecordsFileC, ios::binary | ios::app);
    ofstream metadataFileC(pathMetadataFileC, ios::binary | ios::app);

    if (!recordsFileC || !metadataFileC) {
        throw runtime_error("Error al abrir los archivos");
    }

    DataBaseManager::ValueType type = DataBaseManager::ValueType::STRING_UTF32;

    // Escribir type (1 byte)
    recordsFileC.write(reinterpret_cast<const char*>(&type), sizeof(uint8_t));

    // Obtener posición inicial del value (después del type)
    const uint64_t pos = recordsFileC.tellp();

    // Escribir value (UTF-32)
    recordsFileC.write(
        reinterpret_cast<const char*>(value.data()),
        value.size() * sizeof(char32_t)
    );

    uint64_t amount = 1;
    // Escribir metadatos 
    metadataFileC.write(reinterpret_cast<const char*>(&len), sizeof(uint64_t));
    metadataFileC.write(reinterpret_cast<const char*>(&amount), sizeof(uint64_t));
    metadataFileC.write(reinterpret_cast<const char*>(&pos), sizeof(uint64_t));

    // Debug
    cout << "Type: " << static_cast<int>(type)
        << " - Value: " << utf32ToUtf8(value)
        << " - len: "<<len
        << " - Amount: " << amount
        << " - Pos: " << pos << endl;

    recordsFileC.close();
    metadataFileC.close();
}



int SpecialTokens::insertSpecialTokensInFileC(){

    // Misma carga que el corpus, solo cambia el campo del JSON. Antes esta
    // clase tenia su propia copia de getPromptUtf8 y de how_many_prompts,
    // identicas a las de tokenizerHandler salvo por esa cadena.
    const vector<string> tokens = tokenizerHandler::loadCorpus(specialTokens, "special_token");

    for (const string& tokenUtf8 : tokens){
        try{
            insertInFileC(utf8_to_utf32(tokenUtf8));
        } catch (const exception& e){
            cout<<"Error: "<<e.what()<<endl;
        }
    }

    return 0;
}