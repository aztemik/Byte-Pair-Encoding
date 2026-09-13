
#include <exception>
#include <iostream>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <sys/resource.h>
#include <sys/types.h>

#include "headers/BinaryFileHandler.h"
#include "headers/DataBaseManager.h"
#include "headers/paths.h"
#include "headers/tokenizer.h"
#include "headers/converts.h"
// #include "headers/memoryMonitor.h"
#include "headers/build_backups.h"

using namespace std;

void createAndBackup_Files_A(
    vector<u32string> onlyPairs
){

    try{
        // CREATE FILES A
        BuildBackups::hacerCopiaSeguridad(pathRecordsFileA, all_pares);
        BuildBackups::hacerCopiaSeguridad(pathMetadataFileA, all_pares_metadata);
        cout<<"BACKUP FILES A - EXITOSAMENTE"<<endl;
        BinaryFileAHandler::createFiles_A(onlyPairs);
        cout<<"CREATE FILES A - EXITOSAMENTE"<<"\n"<<endl;
    } catch (const exception& e){
        cerr<<"Error: "<<e.what()<<endl;
    }

};

void createAndBackup_Files_B(
    const vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>>& allDataFilesA
){
    try{
        BuildBackups::hacerCopiaSeguridad(pathRecordsFileB, frequency_valhalla);
        BuildBackups::hacerCopiaSeguridad(pathMetadataFileB, frequency_valhalla_metadata);
        cout<<"BACKUP FILES B - EXITOSAMENTE"<<endl;
        BinaryFileBHandler::createFiles_B(allDataFilesA);
        cout<<"CREATE FILES B - EXITOSAMENTE"<<"\n"<<endl;
    } catch (const exception& e){
        cerr<<"Error: "<<e.what()<<endl;
    }
}

void createAndBackup_Files_C(
    const vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>>& vocabularioFinal
){
    try{
        BuildBackups::hacerCopiaSeguridad(pathRecordsFileC, vocabulary);
        BuildBackups::hacerCopiaSeguridad(pathMetadataFileC, vocabulary_metadata);
        cout<<"BACKUP FILES C - EXITOSAMENTE"<<endl;
        BinaryFileCHandler::createFiles_C(vocabularioFinal);
        cout<<"CREATE FILES C - EXITOSAMENTE"<<"\n"<<endl;
    } catch (const exception& e){
        cerr<<"Error: "<<e.what()<<endl;
    }
}

void createAndBackup_Files_D(const vector<DataBaseManager::mergeRule>& reglas){
    try{
        BuildBackups::hacerCopiaSeguridad(pathRecordsFileD, merges);
        BuildBackups::hacerCopiaSeguridad(pathMetadataFileD, merges_metadata);
        cout<<"BACKUP FILES D - EXITOSAMENTE"<<endl;
        BinaryFileDHandler::createFiles_D(reglas);
        cout<<"CREATE FILES D - EXITOSAMENTE"<<"\n"<<endl;
    } catch (const exception& e){
        cerr<<"Error: "<<e.what()<<endl;
    }
}

// Carga el corpus y lo deja partido en simbolos de un caracter, listo para
// el bucle de fusion. Una frase por elemento: los pares no cruzan de una a otra.
vector<vector<u32string>> cargarSecuencias (const string& path){

    const vector<string> frases = tokenizerHandler::loadCorpus(path);

    cout<<"\nCORPUS: "<< path <<endl;
    cout<<"FRASES: "<< frases.size() <<endl<<"\n";

    // Una secuencia por PALABRA, no por frase: las fusiones no deben cruzar
    // fronteras de palabra.
    vector<vector<u32string>> secuencias;
    for (const string& frase : frases){
        vector<vector<u32string>> palabras =
            tokenizerHandler::pretokenizarEnPalabras(utf8_to_utf32(frase));
        secuencias.insert(secuencias.end(), palabras.begin(), palabras.end());
    }
    cout<<"PALABRAS: "<< secuencias.size() <<endl<<"\n";
    return secuencias;
}

// Todos los pares contiguos del corpus inicial, con repeticiones. Es lo que
// alimenta el almacen A, igual que antes, solo que ahora se construye desde
// las secuencias ya pretokenizadas en vez de releer el JSON por cada frase.
vector<u32string> paresIniciales (const vector<vector<u32string>>& secuencias){
    vector<u32string> pares;
    for (const auto& secuencia : secuencias){
        if (secuencia.size() < 2) continue;
        for (size_t i = 0; i + 1 < secuencia.size(); ++i){
            pares.push_back(secuencia[i] + secuencia[i+1]);
        }
    }
    return pares;
}

// Construye el vocabulario final en el formato que espera createFiles_C.
//
// El orden determina el ID de cada token, asi que se fija de forma explicita:
// primero los caracteres sueltos del corpus (hacen falta siempre, aunque el
// entrenamiento los haya fusionado casi todos, para poder tokenizar texto
// nuevo), y despues los tokens fusionados en el mismo orden en que se
// aprendieron.
vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> construirVocabulario(
    const vector<vector<u32string>>& secuenciasIniciales,
    const vector<vector<u32string>>& secuenciasFinales,
    const vector<DataBaseManager::mergeRule>& reglas
){
    const uint8_t type = static_cast<uint8_t>(DataBaseManager::ValueType::STRING_UTF32);

    // Frecuencia de cada simbolo en el corpus ya fusionado.
    map<u32string, uint64_t> frecuenciaFinal;
    for (const auto& secuencia : secuenciasFinales)
        for (const auto& simbolo : secuencia) frecuenciaFinal[simbolo]++;

    // Caracteres base, por frecuencia descendente en el corpus original.
    map<u32string, uint64_t> frecuenciaBase;
    for (const auto& secuencia : secuenciasIniciales)
        for (const auto& simbolo : secuencia) frecuenciaBase[simbolo]++;

    vector<pair<u32string, uint64_t>> base(frecuenciaBase.begin(), frecuenciaBase.end());
    sort(base.begin(), base.end(),
         [](const auto& a, const auto& b){ return a.second > b.second; });

    vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> vocabulario;
    set<u32string> yaInsertado;

    auto anadir = [&](const u32string& valor, uint64_t amount){
        if (valor.empty() || yaInsertado.count(valor)) return;
        yaInsertado.insert(valor);
        DataBaseManager::operationsFileB operacion{ type, valor, valor.size() * sizeof(char32_t) };
        vocabulario.push_back({ {type, valor}, { operacion, amount } });
    };

    for (const auto& [simbolo, freq] : base) anadir(simbolo, freq);
    for (const auto& regla : reglas){
        const u32string token = regla.resultado();
        anadir(token, frecuenciaFinal.count(token) ? frecuenciaFinal.at(token) : 0);
    }

    return vocabulario;
}

void repeatProccess (){
    int seguimiento;

    do{

        cout<<"1. Read Two Binary Files C\n"
        <<"2. Read Two Binary Files A\n"
        <<"3. Read Two Binary Files B\n"
        <<"4. Salir"<<endl;
        cin>>seguimiento;

        if (seguimiento == 1){
            BinaryFileCHandler::readTwoBinaryFles_C();
        }else if(seguimiento == 2){
            BinaryFileAHandler::readTwoBinaryFiles_A();
        } else if(seguimiento == 3){
            BinaryFileBHandler::readTwoBinaryFiles_B();
        } else if (seguimiento == 4){
            seguimiento = -1;
        }else{
            cout<<"Elige una opcion valida"<<endl;
        }

    } while (seguimiento > 0);

    

};

int main (){

    // Ejemplo: limitar a 6GB (dejando 4GB para el sistema)
    // const size_t SIX_GB = 6ULL * 1024 * 1024 * 1024;
        
    // if (!MemoryMonitor::setMemoryLimit(SIX_GB)) {
    //     std::cerr << "No se pudo establecer el límite de memoria" << std::endl;
    //     return 1;
    // } else{
    //     cout<<"Se establecio el limite de memoria a: "<<SIX_GB<<endl;
    // }

    int opcion;
    cout<<"Opcion: "<<endl;
    cout<<"1. CREATE MERGE RULES\n"
        <<"2. List vocabulary\n"
        <<"3. List Binary/Metadata B\n"
        <<"4. List Binary/Metadata A\n"
        <<"5. Tokenizar text\n"
        <<"6. Insert especial tokens\n"
        <<"7. List merge rules"<<endl;
    cin>>opcion; 

    if (opcion == 1){
        int pathToTraining;
        cout<<"1. schedule\n"
            <<"2. planning\n"
            <<"3. fee\n"
            <<"4. special Tokens"
            <<endl;
        cin>>pathToTraining;
        
        switch (pathToTraining) {
            case 1: {
                cout<<"\n === ENTRENAMIENTO BYTE-PAIR ENCODING === "<<"\n";

                int numeroDeFusiones = 300;
                cout<<"Numero de fusiones a aprender (Enter = 300): ";
                string linea;
                getline(cin >> ws, linea);
                if (!linea.empty()){
                    try { numeroDeFusiones = stoi(linea); } catch (const exception&) {}
                }

                // 1. Corpus partido en caracteres (el JSON se parsea una sola vez)
                vector<vector<u32string>> secuencias = cargarSecuencias(training_schedule);
                if (secuencias.empty()){
                    cerr<<"Corpus vacio o ilegible. Se cancela el entrenamiento."<<endl;
                    break;
                }
                const vector<vector<u32string>> secuenciasIniciales = secuencias;

                // 2. Almacenes A y B: pares en crudo y conteo ordenado.
                //    Se escriben una sola vez, con el estado inicial del corpus.
                createAndBackup_Files_A(paresIniciales(secuencias));

                vector<pair<DataBaseManager::KeyType, DataBaseManager::countOperationFileB>> allDataFilesA;
                try {
                    allDataFilesA = BinaryFileBHandler::loadTwoFiles_A();
                    cout<<"TWO FILES A LOADED INTO RAM\n"<<endl;
                } catch (const exception& e) {
                    cerr<<"Error: "<<e.what()<<endl;
                }
                createAndBackup_Files_B(allDataFilesA);

                // 3. El bucle de fusion: aqui es donde esto pasa de ser una
                //    tabla de bigramas a ser byte-pair encoding.
                cout<<"\n--- BUCLE DE FUSION ---"<<endl;
                vector<DataBaseManager::mergeRule> reglas =
                    tokenizerHandler::trainBPE(secuencias, numeroDeFusiones, 2);
                cout<<"REGLAS APRENDIDAS: "<<reglas.size()<<"\n"<<endl;

                // 4. Almacenes C y D: vocabulario final y reglas de fusion.
                auto vocabularioFinal = construirVocabulario(secuenciasIniciales, secuencias, reglas);
                createAndBackup_Files_C(vocabularioFinal);
                createAndBackup_Files_D(reglas);

                cout<<"TAMANO DEL VOCABULARIO: "<<vocabularioFinal.size()<<endl;
                break;
            }

            case 2: {
                
                break;
            };

            case 3: {
                
                break;
            };        
        }

    } else if (opcion == 2){
        cout<<"\n";
        BinaryFileCHandler::readTwoBinaryFles_C();
    } else if (opcion == 3){
        cout<<"\n\n";
        BinaryFileBHandler::readTwoBinaryFiles_B();
    } else if (opcion == 4){
        BinaryFileAHandler::readTwoBinaryFiles_A();
    } else if (opcion == 5){
        string texto;
        cout<<"Texto a tokenizar (Enter = frase de ejemplo): ";
        getline(cin >> ws, texto);
        if (texto.empty()){
            texto = "Transporte público cercano que me lleve a la estación de tren Buenavista";
            cout<<"Usando: "<<texto<<endl;
        }
        cout<<endl;

        try {
            tokenizerHandler::tokenizer(texto);
        } catch (const exception& e){
            cerr<<"Error: "<<e.what()<<endl;
        }
    } else if(opcion == 6){
        SpecialTokens::insertSpecialTokensInFileC();
    } else if(opcion == 7){
        BinaryFileDHandler::readTwoBinaryFiles_D();
    }

    return 0;
}

