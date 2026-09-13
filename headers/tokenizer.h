#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <cstddef>
#include <string>
#include <vector>
#include "unordered_map"
#include "DataBaseManager.h"

class tokenizerHandler{
    public:

    static std::unordered_map<std::u32string, int> loadVocabularyForTokenizer();

    static std::vector<uint64_t> tokenizer(
        std::string texto
    
    );

    static std::pair<std::pair<std::u32string, std::u32string>, int> maxPar(
        std::vector<DataBaseManager::parYFrecuencia> paresYFrecuencia
    );

    // --- Entrenamiento byte-pair encoding ---

    // Parsea el JSON una sola vez y devuelve el valor de `campo` en cada objeto.
    static std::vector<std::string> loadCorpus(
        const std::string& path,
        const std::string& campo = "prompt"
    );

    // Parte en palabras y cada palabra en caracteres. El espacio se marca como
    // U+E000 y se pega al inicio de la palabra siguiente. Una secuencia por
    // palabra: asi las fusiones no cruzan fronteras de palabra.
    static std::vector<std::vector<std::u32string>> pretokenizarEnPalabras(
        const std::u32string& prompt
    );

    // Cuenta los pares contiguos de todas las secuencias.
    static std::vector<DataBaseManager::parYFrecuencia> contarPares(
        const std::vector<std::vector<std::u32string>>& secuencias
    );

    // Sustituye el par por el token fusionado, sin solape. Devuelve cuantas veces.
    static std::size_t fusionarEnSecuencia(
        std::vector<std::u32string>& secuencia,
        const std::u32string& par1,
        const std::u32string& par2
    );

    // El bucle de fusion. Devuelve las reglas en orden de aprendizaje.
    static std::vector<DataBaseManager::mergeRule> trainBPE(
        std::vector<std::vector<std::u32string>>& secuencias,
        int numeroDeFusiones,
        int umbralDeFrecuencia,
        bool verbose = true
    );

    // Aplica las reglas ya aprendidas, en orden.
    static void aplicarReglas(
        std::vector<std::u32string>& simbolos,
        const std::vector<DataBaseManager::mergeRule>& reglas
    );

};


#endif /* end of TOKENIZER_H */