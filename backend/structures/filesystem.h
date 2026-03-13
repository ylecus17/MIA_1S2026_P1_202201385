#ifndef EXT2_H
#define EXT2_H

#include <cstdint>

// ---------------- SUPER BLOQUE ----------------
// Representa la metadata del sistema de archivos EXT2
struct SuperBloque {
    int32_t SFileSystemType;   // Valor 2 para EXT2
    int32_t SInodesCount;
    int32_t SBlocksCount;
    int32_t SFreeBlocksCount;
    int32_t SFreeInodesCount;
    char    SMtime[20];        // Última fecha de montaje
    char    SUmtime[20];       // Última fecha de desmontaje
    int32_t SMntCount;
    int32_t SMagic;            // 0xEF53
    int32_t SInodeSize;
    int32_t SBlockSize;
    int32_t SFirstIno;         // Primer inodo libre
    int32_t SFirstBlo;         // Primer bloque libre
    int32_t SBitmapInodeStart;
    int32_t SBitmapBlockStart;
    int32_t SInodeStart;
    int32_t SBlockStart;
};

// ---------------- INODO ----------------
struct Inodo {
    int32_t IUid;
    int32_t IGid;
    int32_t ISize;
    char    IAtime[20];
    char    ICtime[20];
    char    IMtime[20];
    int32_t IBlock[15];   // 12 directos, 3 indirectos
    char    IType;        // 0 = carpeta, 1 = archivo
    char    IPerm[3];     // permisos UGO en octal, ej: "664"
};

// ---------------- BLOQUE CARPETA ----------------
struct Content {
    char    BName[12];    // Nombre de archivo/carpeta (máx 12 chars)
    int32_t BInodo;       // Apuntador al inodo
};

struct BloqueCarpeta {
    Content BContent[4];  // Cada bloque almacena hasta 4 entradas
};

// ---------------- BLOQUE ARCHIVO ----------------
struct BloqueArchivo {
    char BContent[64];    // Contenido del archivo (64 bytes por bloque)
};

// ---------------- BLOQUE APUNTADOR ----------------
// Sirve para referencias indirectas a otros bloques
struct BloqueApuntador {
    int32_t BPointer[16]; // Apunta a bloques de datos
};

#endif // EXT2_H
