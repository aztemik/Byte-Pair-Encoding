
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <utility>
#include <vector>
#include <map>
#include <fstream>
#include "../libraries/library_nlohmann/include/nlohmann/json.hpp"
#include "../headers/tokenizer.h"
#include "../headers/paths.h"
#include "../headers/converts.h"
#include "../headers/BinaryFileHandler.h"
#include <algorithm>

using namespace std;
using json = nlohmann::json;

pair<pair<u32string, u32string>, int> tokenizerHandler::maxPar(
    vector<DataBaseManager::parYFrecuencia> paresYFrecuencia
){

    pair<pair<u32string, u32string>, int> parMasFrecuente = {{U"",U"" }, 0};

    for (const auto& c: paresYFrecuencia){
        pair<u32string, u32string> par = {c.par1, c.par2};
        if (c.frecuencia > parMasFrecuente.second){
            parMasFrecuente.first = par;
            parMasFrecuente.second = c.frecuencia;            
        }
    }
    
    return parMasFrecuente;
};




// ---------------------------------------------------------------------------
// Carga del corpus
// ---------------------------------------------------------------------------

// Parsea el JSON UNA sola vez y devuelve el valor de `campo` en cada objeto.
//
// Antes getPromptUtf8(i, path) abria y parseaba el archivo entero para
// devolver un unico elemento, y se llamaba dentro del bucle de entrenamiento:
// con 10 000 frases el corpus se parseaba 10 000 veces. Esa era, con
// diferencia, la parte mas lenta del programa.
//
// El campo es un parametro porque habia dos copias identicas de esta funcion,
// una en tokenizerHandler y otra en SpecialTokens, que solo se diferenciaban
// en si buscaban "prompt" o "special_token". Lo mismo pasaba con las dos
// how_many_prompts: el numero de elementos es ahora frases.size().
vector<string> tokenizerHandler::loadCorpus(const string& path, const string& campo){

    vector<string> frases;

    ifstream inputFile(path);
    if (!inputFile.is_open()) {
        cerr << "Error al abrir el corpus: " << path << endl;
        return frases;
    }

    json j;
    try {
        inputFile >> j;
    } catch (const exception& e) {
        cerr << "Error al parsear el corpus: " << e.what() << endl;
        return frases;
    }

    if (!j.is_array()) {
        cerr << "El JSON no es un array." << endl;
        return frases;
    }

    for (const auto& objeto : j) {
        if (objeto.contains(campo) && objeto[campo].is_string()) {
            string frase = objeto[campo].get<string>();
            if (!frase.empty()) frases.push_back(frase);
        }
    }

    return frases;
}


// Parte la frase en simbolos de un caracter y sustituye el espacio por U+E000.
// El espacio se marca aqui, en la pretokenizacion, y no al formar los pares:
// asi el espacio es un simbolo mas y puede acabar fusionado dentro de un token,
// que es lo que permite aprender cosas como " de" o " la".
// Parte la frase en PALABRAS, y cada palabra en simbolos de un caracter.
//
// Devolver una secuencia por palabra y no una por frase es lo que impide que
// las fusiones crucen fronteras de palabra. Sin esta separacion el bucle
// aprende tokens como "ala" (de "a la") o "delautobu": comprimen bien en el
// corpus de entrenamiento porque memorizan que esas palabras van juntas, pero
// no son unidades del idioma y no generalizan a texto nuevo.
//
// El espacio no se descarta: se convierte en U+E000 y se pega al INICIO de la
// palabra que lo sigue, igual que GPT-2 hace con su simbolo de espacio. Asi el
// tokenizador distingue "de" al principio de frase de " de" en medio, y puede
// reconstruir el texto original sin perder informacion.
vector<vector<u32string>> tokenizerHandler::pretokenizarEnPalabras(const u32string& prompt){
    const char32_t replacement_char = U'\uE000';

    vector<vector<u32string>> palabras;
    vector<u32string> actual;

    for (char32_t c : prompt){
        if (c == U'\u0020'){
            // Cerrar la palabra en curso y empezar la siguiente con el espacio.
            if (!actual.empty()) palabras.push_back(actual);
            actual.clear();
            actual.push_back(u32string(1, replacement_char));
        } else {
            actual.push_back(u32string(1, c));
        }
    }
    if (!actual.empty()) palabras.push_back(actual);

    return palabras;
}

// ---------------------------------------------------------------------------
// Bucle de fusion (byte-pair encoding propiamente dicho)
// ---------------------------------------------------------------------------

// Cuenta los pares contiguos de todas las secuencias. Los pares no cruzan la
// frontera entre frases: cada frase se cuenta por separado.
vector<DataBaseManager::parYFrecuencia> tokenizerHandler::contarPares(
    const vector<vector<u32string>>& secuencias
){
    map<pair<u32string, u32string>, int> frecuencias;
    vector<pair<u32string, u32string>> orden;

    for (const auto& secuencia : secuencias){
        if (secuencia.size() < 2) continue;
        for (size_t i = 0; i + 1 < secuencia.size(); ++i){
            pair<u32string, u32string> par = {secuencia[i], secuencia[i+1]};
            if (frecuencias[par] == 0) orden.push_back(par);
            frecuencias[par]++;
        }
    }

    vector<DataBaseManager::parYFrecuencia> resultado;
    resultado.reserve(orden.size());
    for (const auto& par : orden){
        resultado.emplace_back(par.first, par.second, frecuencias[par]);
    }
    return resultado;
}


// Sustituye en la secuencia todas las apariciones del par por el token nuevo.
// El recorrido es de izquierda a derecha y sin solape: tras fusionar dos
// simbolos se salta por encima de ambos, asi "aaa" con la regla (a,a) da
// ["aa", "a"] y no ["aa", "aa"].
size_t tokenizerHandler::fusionarEnSecuencia(
    vector<u32string>& secuencia,
    const u32string& par1,
    const u32string& par2
){
    if (secuencia.size() < 2) return 0;

    const u32string tokenNuevo = par1 + par2;
    vector<u32string> salida;
    salida.reserve(secuencia.size());
    size_t fusiones = 0;

    size_t i = 0;
    while (i < secuencia.size()){
        if (i + 1 < secuencia.size() && secuencia[i] == par1 && secuencia[i+1] == par2){
            salida.push_back(tokenNuevo);
            i += 2;              // saltar los dos simbolos consumidos
            ++fusiones;
        } else {
            salida.push_back(secuencia[i]);
            ++i;
        }
    }

    secuencia.swap(salida);
    return fusiones;
}


// El bucle de entrenamiento. Es el algoritmo entero de byte-pair encoding:
// contar, fusionar el par mas frecuente, sustituirlo en el corpus y repetir.
// Cada vuelta puede formar pares entre tokens ya fusionados, que es lo que hace
// que el vocabulario crezca mas alla de pares de caracteres.
vector<DataBaseManager::mergeRule> tokenizerHandler::trainBPE(
    vector<vector<u32string>>& secuencias,
    int numeroDeFusiones,
    int umbralDeFrecuencia,
    bool verbose
){
    vector<DataBaseManager::mergeRule> reglas;

    for (int vuelta = 0; vuelta < numeroDeFusiones; ++vuelta){

        vector<DataBaseManager::parYFrecuencia> pares = contarPares(secuencias);
        if (pares.empty()){
            if (verbose) cout << "No quedan pares que fusionar. Fin en la vuelta " << vuelta << ".\n";
            break;
        }

        // maxPar() ya existia en el repositorio pero no se llamaba desde
        // ningun sitio. Este es su sitio.
        pair<pair<u32string, u32string>, int> mejor = maxPar(pares);

        if (mejor.second < umbralDeFrecuencia){
            if (verbose){
                cout << "El par mas frecuente aparece " << mejor.second
                     << " veces, por debajo del umbral " << umbralDeFrecuencia
                     << ". Fin en la vuelta " << vuelta << ".\n";
            }
            break;
        }

        const u32string& par1 = mejor.first.first;
        const u32string& par2 = mejor.first.second;

        for (auto& secuencia : secuencias){
            fusionarEnSecuencia(secuencia, par1, par2);
        }

        reglas.push_back(DataBaseManager::mergeRule{par1, par2});

        if (verbose && (vuelta < 10 || vuelta % 50 == 0)){
            cout << "  fusion #" << vuelta << ": ("
                 << tokenParaMostrar(par1) << " + " << tokenParaMostrar(par2) << ") -> "
                 << tokenParaMostrar(par1 + par2)
                 << "   [" << mejor.second << " apariciones]" << endl;
        }
    }

    return reglas;
}


// Aplica las reglas ya aprendidas, en el mismo orden en que se aprendieron.
// Ese orden es obligatorio: una regla tardia puede depender de un token que
// solo existe porque una regla anterior lo creo.
void tokenizerHandler::aplicarReglas(
    vector<u32string>& simbolos,
    const vector<DataBaseManager::mergeRule>& reglas
){
    for (const auto& regla : reglas){
        fusionarEnSecuencia(simbolos, regla.par1, regla.par2);
    }
}


unordered_map<u32string, int> tokenizerHandler::loadVocabularyForTokenizer(

) {

    vector<u32string> vocab_vector;
    unordered_map<u32string, int> value_to_tokenID;
    
    ifstream recordsFileC(pathRecordsFileC, ios::binary);
    ifstream metadataFileC(pathMetadataFileC, ios::binary);

    if (!metadataFileC.is_open() || !recordsFileC.is_open()) {
        cerr << "Error al abrir los archivos\n";
    }

    DataBaseManager::struct_metadataFileC metadata;
    while (metadataFileC.read(reinterpret_cast<char*>(&metadata), sizeof(DataBaseManager::struct_metadataFileC))) {

        // Posicionarse al inicio del value
        recordsFileC.seekg(metadata.pos);

        // Leer value (UTF-32)
        vector<char32_t> value(metadata.len / sizeof(char32_t));
        recordsFileC.read(reinterpret_cast<char*>(value.data()), metadata.len);

        // Construir desde iteradores: value no lleva terminador nulo, asi que
        // el constructor desde puntero leeria mas alla del buffer.
        u32string valor(value.begin(), value.end());

        // insertar merge rule en vector
        vocab_vector.push_back(valor);
        // insertar idRecord y relacionar con la posicion del value en el vector
        value_to_tokenID[valor] = vocab_vector.size() - 1;


        // Debug
        // param_vectorChar32_see(value);
        // cout<<"\n";

    }
    
    metadataFileC.close();
    recordsFileC.close();

    return value_to_tokenID;
};

// Tokeniza aplicando las reglas de fusion aprendidas, en orden.
//
// La version anterior emitia todos los pares contiguos SOLAPADOS del texto:
// para 10 caracteres emitia 9 identificadores y cada letra del medio quedaba
// codificada dos veces. Eso no comprime nada, que es justo para lo que existe
// byte-pair encoding. Ahora el texto se parte en simbolos, se le aplican las
// reglas y cada simbolo resultante produce exactamente un identificador.
vector<uint64_t> tokenizerHandler::tokenizer(string texto) {

    unordered_map<u32string, int> value_to_tokenID = loadVocabularyForTokenizer();
    const vector<DataBaseManager::mergeRule> reglas = BinaryFileDHandler::loadMergeRules();

    const u32string unk_token = U"<UNK>";
    auto unk_it = value_to_tokenID.find(unk_token);
    if (unk_it == value_to_tokenID.end()) {
        throw runtime_error("Token <UNK> no encontrado en el vocabulario. "
                            "Ejecuta antes la opcion de insertar tokens especiales.");
    }
    const uint64_t unk_id = unk_it->second;

    const u32string textoUTF32 = utf8_to_utf32(texto);
    const size_t caracteres = textoUTF32.size();

    // Las reglas se aplican dentro de cada palabra por separado, igual que en
    // el entrenamiento: si aqui se aplicaran sobre la frase entera, el
    // tokenizador fusionaria a traves de los espacios y no coincidiria con lo
    // que se aprendio.
    vector<vector<u32string>> palabras = pretokenizarEnPalabras(textoUTF32);

    vector<u32string> simbolos;
    for (auto& palabra : palabras){
        aplicarReglas(palabra, reglas);
        simbolos.insert(simbolos.end(), palabra.begin(), palabra.end());
    }

    vector<uint64_t> texto_tokenizado;
    texto_tokenizado.reserve(simbolos.size());

    for (const auto& simbolo : simbolos) {
        auto it = value_to_tokenID.find(simbolo);
        const uint64_t id = (it != value_to_tokenID.end()) ? uint64_t(it->second) : unk_id;
        texto_tokenizado.push_back(id);

        cout << "[" << tokenParaMostrar(simbolo) << "]";
        if (it == value_to_tokenID.end()) cout << "(UNK)";
    }
    cout << "\n\n";

    cout << "Caracteres de entrada: " << caracteres << "\n";
    cout << "Tokens de salida:      " << texto_tokenizado.size() << "\n";
    if (texto_tokenizado.size() > 0){
        cout << "Compresion:            "
             << double(caracteres) / double(texto_tokenizado.size()) << " caracteres por token\n";
    }
    cout << "Reglas de fusion aplicadas: " << reglas.size() << endl;

    return texto_tokenizado;
}
