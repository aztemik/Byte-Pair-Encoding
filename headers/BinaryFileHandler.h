#ifndef BINARYFILEHANDLER_H
#define BINARYFILEHANDLER_H

#include <cstdint>
#include <fstream>
#include <string>
#include "DataBaseManager.h"
#include <unordered_map>
#include <vector>



class BinaryFileAHandler {

    public:
    static int createFiles_A (const std::vector<std::u32string>& onlyPares);

    static void insertRecordsToFileA(
        const std::u32string& value,
        std::ofstream& recordsFile,
        std::ofstream& metadataFile,
        uint64_t& offset
    );

    static void readTwoBinaryFiles_A ();
};


class BinaryFileBHandler {

    public:
    static std::vector<std::pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> loadTwoFiles_A();

    static void mostrarVector(
        std::vector<std::pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> VectorOrdenadoYAllData
    );

    static void createFiles_B (
        const std::vector<std::pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>>&  allDataForFiles_B
    );

    static void readTwoBinaryFiles_B ();

};

class BinaryFileCHandler{

    public: 
    static void createFiles_C(std::vector<std::pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> allData);

    static void readTwoBinaryFles_C();


};

// Almacen D: reglas de fusion en orden de aprendizaje.
class BinaryFileDHandler{

    public:
    static void createFiles_D(const std::vector<DataBaseManager::mergeRule>& reglas);

    static std::vector<DataBaseManager::mergeRule> loadMergeRules();

    static void readTwoBinaryFiles_D();

};

class SpecialTokens{

    public:

    static void insertInFileC(
        std::u32string value
    );

    static int insertSpecialTokensInFileC();

};



#endif /* end of BINARYFILEHANDLER_H */