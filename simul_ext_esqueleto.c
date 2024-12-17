#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100

// Declaración de funciones
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *bytemaps,
           EXT_SIMPLE_SUPERBLOCK *superblock, char *nombre, FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *bytemaps,
           EXT_SIMPLE_SUPERBLOCK *superblock, EXT_DATOS *memdatos, char *origen, char *destino, FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main() {
    char comando[LONGITUD_COMANDO];
    char orden[LONGITUD_COMANDO];
    char argumento1[LONGITUD_COMANDO];
    char argumento2[LONGITUD_COMANDO];

    EXT_SIMPLE_SUPERBLOCK ext_superblock;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
    int grabardatos = 0;
    FILE *fent;

    fent = fopen("particion.bin", "r+b");
    if (fent == NULL) {
        printf("Error: No se pudo abrir el archivo particion.bin\n");
        return 1;
    }

    fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);
    memcpy(&ext_superblock, &datosfich[0], sizeof(ext_superblock));
    memcpy(&ext_bytemaps, &datosfich[1], sizeof(ext_bytemaps));
    memcpy(&ext_blq_inodos, &datosfich[2], sizeof(ext_blq_inodos));
    memcpy(directorio, &datosfich[3], sizeof(directorio));
    memcpy(memdatos, &datosfich[4], sizeof(memdatos));

    for (;;) {
        printf(">> ");
        fflush(stdin);
        fgets(comando, LONGITUD_COMANDO, stdin);

        if (ComprobarComando(comando, orden, argumento1, argumento2) != 0) {
            printf("Comando desconocido\n");
            continue;
        }

        if (strcmp(orden, "info") == 0) {
            LeeSuperBloque(&ext_superblock);
        } else if (strcmp(orden, "bytemaps") == 0) {
            Printbytemaps(&ext_bytemaps);
        } else if (strcmp(orden, "dir") == 0) {
            Directorio(directorio, &ext_blq_inodos);
        } else if (strcmp(orden, "renombrar") == 0) {
            if (Renombrar(directorio, &ext_blq_inodos, argumento1, argumento2) == 0)
                grabardatos = 1;
        } else if (strcmp(orden, "imprimir") == 0) {
            Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
        } else if (strcmp(orden, "borrar") == 0) {
            if (Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fent) == 0)
                grabardatos = 1;
        } else if (strcmp(orden, "copiar") == 0) {
            if (Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2, fent) == 0)
                grabardatos = 1;
        } else if (strcmp(orden, "salir") == 0) {
            GrabarDatos(memdatos, fent);
            fclose(fent);
            return 0;
        } else {
            printf("Comando desconocido\n");
        }

        Grabarinodosydirectorio(directorio, &ext_blq_inodos, fent);
        GrabarByteMaps(&ext_bytemaps, fent);
        GrabarSuperBloque(&ext_superblock, fent);
        if (grabardatos) {
            GrabarDatos(memdatos, fent);
            grabardatos = 0;
        }
    }
}

// Implementación de funciones

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2) {
    char *token = strtok(strcomando, " \n");
    if (token == NULL) return 1;

    strcpy(orden, token);
    token = strtok(NULL, " \n");
    if (token != NULL) {
        strcpy(argumento1, token);
        token = strtok(NULL, " \n");
        if (token != NULL) {
            strcpy(argumento2, token);
        }
    }
    return 0;
}

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    printf("Información del Superbloque:\n");
    printf(" - Inodos totales: %u\n", psup->s_inodes_count);
    printf(" - Bloques totales: %u\n", psup->s_blocks_count);
    printf(" - Bloques libres: %u\n", psup->s_free_blocks_count);
    printf(" - Inodos libres: %u\n", psup->s_free_inodes_count);
    printf(" - Primer bloque de datos: %u\n", psup->s_first_data_block);
    printf(" - Tamaño de bloque: %u bytes\n", psup->s_block_size);
}

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    printf("Bytemap de bloques (primeros 25):\n");
    for (int i = 0; i < 25; i++) {
        printf("%u ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\n");

    printf("Bytemap de inodos:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%u ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\n");
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    printf("Listado de archivos:\n");
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO && strcmp(directorio[i].dir_nfich, ".") != 0) {
            EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[directorio[i].dir_inodo];
            printf("Archivo: %s, Tamaño: %u bytes, Inodo: %u, Bloques: ", 
                   directorio[i].dir_nfich, inodo->size_fichero, directorio[i].dir_inodo);
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO && inodo->i_nbloque[j] != NULL_BLOQUE; j++) {
                printf("%u ", inodo->i_nbloque[j]);
            }
            printf("\n");
        }
    }
}

int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo) {
    int index_antiguo = -1;
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            if (strcmp(directorio[i].dir_nfich, nombreantiguo) == 0) {
                index_antiguo = i;
            } else if (strcmp(directorio[i].dir_nfich, nombrenuevo) == 0) {
                printf("Error: El nombre %s ya existe.\n", nombrenuevo);
                return -1;
            }
        }
    }
    if (index_antiguo == -1) {
        printf("Error: Archivo %s no encontrado.\n", nombreantiguo);
        return -1;
    }
    strcpy(directorio[index_antiguo].dir_nfich, nombrenuevo);
    return 0;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO && strcmp(directorio[i].dir_nfich, nombre) == 0) {
            EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[directorio[i].dir_inodo];
            printf("Contenido del archivo %s:\n", nombre);
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO && inodo->i_nbloque[j] != NULL_BLOQUE; j++) {
                printf("%s", memdatos[inodo->i_nbloque[j]].dato);
            }
            printf("\n");
            return 0;
        }
    }
    printf("Error: Archivo %s no encontrado.\n", nombre);
    return -1;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *bytemaps,
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre, FILE *fich) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO && strcmp(directorio[i].dir_nfich, nombre) == 0) {
            EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[directorio[i].dir_inodo];
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO && inodo->i_nbloque[j] != NULL_BLOQUE; j++) {
                bytemaps->bmap_bloques[inodo->i_nbloque[j]] = 0;
                inodo->i_nbloque[j] = NULL_BLOQUE;
            }
            inodo->size_fichero = 0;
            bytemaps->bmap_inodos[directorio[i].dir_inodo] = 0;
            directorio[i].dir_inodo = NULL_INODO;
            strcpy(directorio[i].dir_nfich, "");
            ext_superblock->s_free_inodes_count++;
            ext_superblock->s_free_blocks_count++;
            return 0;
        }
    }
    printf("Error: Archivo %s no encontrado.\n", nombre);
    return -1;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *bytemaps,
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *origen, char *destino, FILE *fich) {
    int inodo_origen = -1, inodo_destino = -1;
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO && strcmp(directorio[i].dir_nfich, origen) == 0) {
            inodo_origen = directorio[i].dir_inodo;
        }
        if (directorio[i].dir_inodo != NULL_INODO && strcmp(directorio[i].dir_nfich, destino) == 0) {
            printf("Error: El nombre %s ya existe.\n", destino);
            return -1;
        }
    }
    if (inodo_origen == -1) {
        printf("Error: Archivo %s no encontrado.\n", origen);
        return -1;
    }

    // Buscar un inodo libre
    for (int i = 0; i < MAX_INODOS; i++) {
        if (bytemaps->bmap_inodos[i] == 0) {
            inodo_destino = i;
            break;
        }
    }
    if (inodo_destino == -1) {
        printf("Error: No hay inodos libres.\n");
        return -1;
    }

    // Copiar contenido del archivo
    EXT_SIMPLE_INODE *origen_inodo = &inodos->blq_inodos[inodo_origen];
    EXT_SIMPLE_INODE *destino_inodo = &inodos->blq_inodos[inodo_destino];
    destino_inodo->size_fichero = origen_inodo->size_fichero;
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO && origen_inodo->i_nbloque[i] != NULL_BLOQUE; i++) {
        for (int j = PRIM_BLOQUE_DATOS; j < MAX_BLOQUES_PARTICION; j++) {
            if (bytemaps->bmap_bloques[j] == 0) {
                bytemaps->bmap_bloques[j] = 1;
                destino_inodo->i_nbloque[i] = j;
                memcpy(memdatos[j].dato, memdatos[origen_inodo->i_nbloque[i]].dato, SIZE_BLOQUE);
                break;
            }
        }
    }
    bytemaps->bmap_inodos[inodo_destino] = 1;
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo == NULL_INODO) {
            strcpy(directorio[i].dir_nfich, destino);
            directorio[i].dir_inodo = inodo_destino;
            break;
        }
    }
    ext_superblock->s_free_inodes_count--;
    ext_superblock->s_free_blocks_count--;
    return 0;
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    fseek(fich, 2 * SIZE_BLOQUE, SEEK_SET);
    fwrite(inodos, SIZE_BLOQUE, 1, fich);
    fseek(fich, 3 * SIZE_BLOQUE, SEEK_SET);
    fwrite(directorio, SIZE_BLOQUE, 1, fich);
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    fseek(fich, SIZE_BLOQUE, SEEK_SET);
    fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    fseek(fich, 0, SEEK_SET);
    fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
    fseek(fich, PRIM_BLOQUE_DATOS * SIZE_BLOQUE, SEEK_SET);
    fwrite(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);
}
