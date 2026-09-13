#include <cstdint>
#include <string>

#include "../headers/GeneralBinaryFileHandler.h"

using namespace std;

// Numero de bytes que ocupan unos caracteres UTF-32: 4 por caracter.
// Se toma la longitud del string en vez de recorrer hasta un terminador nulo,
// que es lo que hacia antes: un valor con un U+0000 dentro se habria contado mal.
uint64_t GeneralBinaryFileHandler::calculateBytesOfCharacters(const u32string& value) {
    return value.size() * sizeof(char32_t);
}
