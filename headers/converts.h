#ifndef CONVERTS
#define CONVERTS

#include <cstdint>
#include <iostream>
#include <string>

// Conversion UTF-8 <-> UTF-32 escrita a mano.
//
// Antes esto usaba std::wstring_convert con std::codecvt_utf8. Las dos estan
// marcadas obsoletas desde C++17 y eliminadas en C++26: cada compilacion
// escupia deprecaciones y el dia que subiera el estandar este archivo dejaba
// de compilar. Son unas pocas decenas de lineas y no hacen falta dependencias.
//
// Las secuencias mal formadas no lanzan: se sustituyen por U+FFFD, el caracter
// de reemplazo. Un corpus con un byte corrupto no debe tumbar el entrenamiento.

inline constexpr char32_t CARACTER_DE_REEMPLAZO = U'\uFFFD';

inline std::u32string utf8_to_utf32(const std::string& input) {
    std::u32string salida;
    salida.reserve(input.size());

    const unsigned char* p = reinterpret_cast<const unsigned char*>(input.data());
    const size_t n = input.size();
    size_t i = 0;

    while (i < n) {
        const unsigned char c = p[i];
        char32_t cp = 0;
        size_t extra = 0;      // bytes de continuacion que siguen
        char32_t minimo = 0;   // menor valor que esta longitud puede codificar

        if (c < 0x80)             { cp = c;        extra = 0; minimo = 0x00000; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; minimo = 0x00080; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; minimo = 0x00800; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; minimo = 0x10000; }
        else {
            // Byte de continuacion suelto o prefijo invalido (0xF8..0xFF).
            salida.push_back(CARACTER_DE_REEMPLAZO);
            ++i;
            continue;
        }

        if (i + extra >= n) {                 // secuencia cortada al final
            salida.push_back(CARACTER_DE_REEMPLAZO);
            break;
        }

        bool valida = true;
        for (size_t k = 1; k <= extra; ++k) {
            const unsigned char cont = p[i + k];
            if ((cont & 0xC0) != 0x80) { valida = false; break; }
            cp = (cp << 6) | (cont & 0x3F);
        }

        // Rechazar: continuacion invalida, codificacion mas larga de lo
        // necesario, sustitutos UTF-16 y valores por encima del maximo Unicode.
        if (!valida || cp < minimo || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
            salida.push_back(CARACTER_DE_REEMPLAZO);
            ++i;
            continue;
        }

        salida.push_back(cp);
        i += extra + 1;
    }

    return salida;
}


inline std::string utf32ToUtf8(const std::u32string& input) {
    std::string salida;
    salida.reserve(input.size());

    for (char32_t cp : input) {
        if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            cp = CARACTER_DE_REEMPLAZO;
        }

        if (cp < 0x80) {
            salida.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            salida.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            salida.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            salida.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            salida.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            salida.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            salida.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            salida.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            salida.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            salida.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    return salida;
}


// El espacio se guarda como U+E000, que es de uso privado y no se ve en la
// terminal. Para mostrar tokens por pantalla se sustituye por un simbolo
// visible, o los tokens con espacio parecen tener un hueco sin explicacion.
inline std::string tokenParaMostrar(const std::u32string& token) {
    std::string salida;
    for (char32_t c : token) {
        if (c == U'\uE000') salida += "\u2423";   // simbolo visible de espacio
        else salida += utf32ToUtf8(std::u32string(1, c));
    }
    return salida;
}

#endif // CONVERTS
