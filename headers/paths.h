#ifndef PHATS_H
#define PHATS_H

#include <string>

// Directorio donde viven los almacenes binarios generados por el entrenamiento.
// Se crea en el repositorio con un .gitkeep para que funcione recien clonado.
inline const std::string normalPath = "data/out/";

inline const std::string pathRecordsFileA = normalPath + "all_pares.bin";
inline const std::string pathMetadataFileA = normalPath + "all_pares_metadata.bin";

inline const std::string all_pares = "all_pares_";
inline const std::string all_pares_metadata = "all_pares_metadata_";

inline const std::string pathRecordsFileB = normalPath + "frequency_valhalla.bin";
inline const std::string pathMetadataFileB = normalPath + "frequency_metadata_valhalla.bin";

inline const std::string frequency_valhalla = "frequency_valhalla_";
inline const std::string frequency_valhalla_metadata = "frequency_valhalla_metadata_";

inline const std::string pathRecordsFileC = normalPath + "vocabulary.bin";
inline const std::string pathMetadataFileC = normalPath + "vocabulary_metadata.bin";

inline const std::string vocabulary = "vocabulary";
inline const std::string vocabulary_metadata = "vocabulary_metadata";

// Almacen D: reglas de fusion en el orden en que se aprendieron.
// El orden importa: al tokenizar se aplican en ese mismo orden.
inline const std::string pathRecordsFileD = normalPath + "merges.bin";
inline const std::string pathMetadataFileD = normalPath + "merges_metadata.bin";

inline const std::string merges = "merges_";
inline const std::string merges_metadata = "merges_metadata_";

// Destino de las copias de seguridad con marca de tiempo.
inline std::string saveAnyChange = "backups/";

// Corpus de entrenamiento: array JSON de objetos con campo "prompt".
inline const std::string training_schedule = "data/corpus_schedule.json";
inline const std::string training_journey = "data/corpus_schedule.json";
inline const std::string training_fee = "data/corpus_schedule.json";
inline const std::string training_planning = "data/corpus_schedule.json";

// Tokens especiales: array JSON de objetos con campo "special_token".
inline const std::string specialTokens = "data/special_tokens.json";

#endif /* end of PHATS_H */
