# Manual Técnico - Sistema de Archivos EXT2 Simulado

## 1. Arquitectura del Sistema

El sistema está dividido en dos componentes principales:

### Backend (C++)
Encargado de:
- Manejo de discos (.mia)
- Implementación de comandos
- Manipulación de estructuras EXT2
- Control de sesiones

### Frontend (Web)
Encargado de:
- Interfaz de usuario
- Envío de comandos al backend
- Visualización de resultados

### Flujo general
Usuario → Frontend → Parser → Comando → Estructuras → Disco (.mia)

---

## 2. Estructuras de Datos

### MBR
```
struct MBR {
    int32_t MbrSize;
    char MbrDate[20];
    int32_t MbrSignature;
    Partition Partitions[4];
};
```

### Partition
```
struct Partition {
    char PartStatus;
    char PartType;
    char PartFit;
    int32_t PartStart;
    int32_t PartSize;
    char PartName[16];
};
```

### SuperBloque
```
struct SuperBloque {
    int32_t SFileSystemType;
    int32_t SInodesCount;
    int32_t SBlocksCount;
    int32_t SFreeBlocksCount;
    int32_t SFreeInodesCount;
    int32_t SInodeStart;
    int32_t SBlockStart;
};
```

### Inodo
```
struct Inodo {
    int32_t IUid;
    int32_t IGid;
    int32_t ISize;
    int32_t IBlock[15];
    char IType;
    char IPerm[3];
};
```

### BloqueCarpeta
```
struct BloqueCarpeta {
    Content BContent[4];
};
```

### BloqueArchivo
```
struct BloqueArchivo {
    char BContent[64];
};
```

---

## 3. Comandos Implementados

### MKDISK
Crea un disco

Ejemplo:
```
mkdisk -size=1024 -path=/home/disco.mia
```

---

### FDISK
Crea particiones

Ejemplo:
```
fdisk -size=512 -path=/home/disco.mia -name=part1
```

---

### MOUNT
Monta una partición

Ejemplo:
```
mount -path=/home/disco.mia -name=part1
```

---

### MKFS
Formatea una partición

Ejemplo:
```
mkfs -id=vda1 -type=full
```

---

### LOGIN
Inicia sesión

Ejemplo:
```
login -user=root -pass=123 -id=vda1
```

---

### LOGOUT
Cierra sesión

Ejemplo:
```
logout
```

---

### MKGRP
Crea grupo

Ejemplo:
```
mkgrp -name=usuarios
```

---

### RMGRP
Elimina grupo

Ejemplo:
```
rmgrp -name=usuarios
```

---

### MKUSR
Crea usuario

Ejemplo:
```
mkusr -user=juan -pass=123 -grp=usuarios
```

---

### RMUSR
Elimina usuario

Ejemplo:
```
rmusr -user=juan
```

---

### MKDIR
Crea carpetas

Ejemplo:
```
mkdir -p -path=/home/user/docs
```


```
bool Mkdir(const std::string& path, bool pFlag) {
    if (!IsLogged()) return false;

    MountedPartition* mp = getMountById(CurrentSesion.ID);
    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);

    SuperBloque sb;
    file.seekg(mp->Start);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    std::string parentPath = getParentPath(path);
    int parent = findFolderInode(file, sb, parentPath);

    if (parent == -1 && pFlag) {
        createParentFolders(file, sb, parentPath);
        parent = findFolderInode(file, sb, parentPath);
    }

    return createFolder(file, sb, parent, getFileName(path));
}
```

---

### MKFILE
Crea archivos

Ejemplo:
```
mkfile -size=15 -path=/home/user/docs/a.txt -r
```

Código:
```
bool Mkfile(const std::string& path, bool rFlag, int size, const std::string& cont) {
    if (!IsLogged()) return false;

    MountedPartition* mp = getMountById(CurrentSesion.ID);
    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);

    SuperBloque sb;
    file.seekg(mp->Start);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    std::string parentPath = getParentPath(path);
    int parent = findFolderInode(file, sb, parentPath);

    if (parent == -1 && rFlag) {
        createParentFolders(file, sb, parentPath);
        parent = findFolderInode(file, sb, parentPath);
    }

    std::string content;
    if (size > 0) {
        for (int i = 0; i < size; i++) content += char('0' + (i % 10));
    }

    return createFile(file, sb, parent, getFileName(path), content);
}
```

---

### CAT
Lee archivos

Ejemplo:
```
cat /home/user/docs/a.txt
```

---

## 4. Manejo de Permisos

Permisos tipo Linux:
- 664 → lectura/escritura propietario
- lectura grupo
- lectura otros

```
strcpy(inode.IPerm, "664");
```

---

## 5. Notas Finales

- El sistema usa archivos binarios .mia
- Se manejan estructuras tipo EXT2
- Todo se basa en offsets dentro del archivo