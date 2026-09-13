#ifndef GENERALBINARYFILEHANDLER_H
#define GENERALBINARYFILEHANDLER_H

#include <cstdint>
#include <string>

class GeneralBinaryFileHandler {

    public:

    static uint64_t calculateBytesOfCharacters(const std::u32string& value);

};

#endif /* end of GENERALBINARYFILEHANDLER_H */
