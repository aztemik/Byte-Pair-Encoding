# Byte-pair encoding: tokenizador en C++

Tokenizador construido desde cero en C++17, sin dependencias más allá de
`nlohmann/json`. Aprende un vocabulario a partir de un corpus propio y convierte
texto en identificadores numéricos.

La persistencia es propia: no hay motor de base de datos detrás, sino cuatro
almacenes binarios escritos a mano, cada uno con su archivo de registros y su
archivo de metadatos de tamaño fijo.

## Qué hace

```
$ ./main
5
Texto a tokenizar: Transporte público cercano a la estación de tren Buenavista

[Transporte][␣público][␣cerca][no][␣a][␣la][␣estación][␣de][␣tren][␣Buenavista]

Caracteres de entrada: 59
Tokens de salida:      10
Compresión:            5.9 caracteres por token
```

El símbolo `␣` marca el espacio que precede a una palabra: forma parte del token,
igual que el `Ġ` de GPT-2. Gracias a eso el tokenizador distingue `de` al
principio de una frase de `␣de` en medio, y el texto original se puede
reconstruir sin pérdida.

## Cómo funciona

### El bucle de fusión

Byte-pair encoding es un bucle: contar los pares contiguos, fusionar el más
frecuente en un token nuevo, sustituirlo en todo el corpus y volver a empezar.
Repitiéndolo, aparecen tokens cada vez más largos.

```
vocabulario = caracteres distintos del corpus
repetir N veces:
    contar pares contiguos
    mejor = par más frecuente
    si frecuencia(mejor) < umbral: parar
    reglas.añadir(mejor -> mejor.primero + mejor.segundo)
    sustituir mejor en el corpus
```

Las **reglas de fusión** se guardan en el orden en que se aprendieron, y ese
orden es parte del algoritmo: al tokenizar se aplican en la misma secuencia,
porque una regla tardía puede depender de un token que solo existe porque una
regla anterior lo creó.

Las fusiones **no cruzan fronteras de palabra**. Cada palabra es una secuencia
independiente. Sin esa restricción el entrenamiento aprende tokens como `ala`
(de «a la») o `delautobú`: comprimen bien en el corpus de entrenamiento porque
memorizan qué palabras van juntas, pero no son unidades del idioma y no
generalizan. Con la restricción, lo que se aprende son morfemas — `ción`,
`␣hora`, `␣transporte` — que sí sirven para texto nuevo.

### Por qué UTF-32 por dentro

El texto entra en UTF-8 y se convierte a UTF-32 para trabajar. En UTF-32 todo
carácter ocupa exactamente cuatro bytes, así que indexar por posición es
directo y no hay que decodificar nada para saber dónde empieza el carácter
siguiente. Eso importa cuando el algoritmo se pasa el rato mirando pares
contiguos. La conversión en los extremos está escrita a mano en `converts.h`.

### Los almacenes binarios

| Almacén | Registros | Metadatos | Contenido |
|---|---|---|---|
| A | `all_pares.bin` | `all_pares_metadata.bin` | Pares en crudo del corpus inicial, con repeticiones |
| B | `frequency_valhalla.bin` | `frequency_metadata_valhalla.bin` | Pares distintos con su frecuencia, ordenados |
| C | `vocabulary.bin` | `vocabulary_metadata.bin` | Vocabulario final; el ID de un token es su posición |
| D | `merges.bin` | `merges_metadata.bin` | Reglas de fusión, en orden de aprendizaje |

Los metadatos son de tamaño fijo y guardan la longitud y el desplazamiento de
cada valor, así que se llega a cualquier registro por acceso directo, sin
recorrer el archivo. El almacén D guarda las dos longitudes del par por separado
en vez de usar un separador entre ellas: un separador podría aparecer dentro de
un token ya fusionado, las longitudes no tienen ese problema.

Antes de reconstruir un almacén se guarda una copia con marca de tiempo en
`backups/`.

## Compilar y ejecutar

```
make
./main
```

Requiere `clang++` y C++17.

El menú es interactivo y el orden importa, porque cada paso lee lo que escribió
el anterior:

| Orden | Opción | Qué hace |
|---|---|---|
| 1º | `1` → `1` | Entrena: construye A y B, ejecuta el bucle de fusión, escribe C y D |
| 2º | `6` | Inserta los tokens especiales (`<UNK>`, `<PAD>`, `<BOS>`, `<EOS>`) en C |
| 3º | `5` | Tokeniza el texto que le escribas |

El `6` va después del `1` y no al revés: el entrenamiento reescribe C entero, así
que unos tokens especiales insertados antes se perderían. Y sin el `6`, la opción
`5` falla: la tokenización necesita que `<UNK>` exista en el vocabulario.

Las opciones `2`, `3`, `4` y `7` listan el contenido de C, B, A y D para
inspeccionarlos.

## Configuración

Las rutas se definen en `headers/paths.h`. El repositorio trae un corpus de
ejemplo de 30 frases para que funcione recién clonado.

El corpus es un array JSON de objetos con campo `prompt`:

```json
[{ "prompt": "Transporte público cercano a la estación Buenavista" }]
```

Los tokens especiales usan el mismo formato con el campo `special_token`.

## Estado

Funciona de principio a fin: entrena, persiste y tokeniza. Es un proyecto de
aprendizaje, no una librería para producción.

Lo que falta:

- [ ] Conteo incremental de pares. Hoy cada vuelta del bucle recuenta el corpus
      entero, que es O(N × corpus). Con el corpus de ejemplo es instantáneo; con
      uno grande conviene mantener los contadores y actualizar solo los pares
      afectados por cada fusión.
- [ ] Configuración por archivo o por argumentos de línea de comandos, en vez de
      constantes en `paths.h`.
- [ ] Pruebas automatizadas.
- [ ] Destokenizar: reconstruir el texto a partir de los identificadores.
