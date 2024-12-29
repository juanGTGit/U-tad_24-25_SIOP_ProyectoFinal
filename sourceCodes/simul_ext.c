#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino);

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
    FILE *fent;

    fent = fopen("particion.bin", "r+b");
    if (fent == NULL) {
        printf("Error: No se pudo abrir el archivo particion.bin\n");
        return -1;
    }

    fread(datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);

    memcpy(&ext_superblock, (EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
    memcpy(&ext_bytemaps, (EXT_BYTE_MAPS *)&datosfich[1], SIZE_BLOQUE);
    memcpy(&ext_blq_inodos, (EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
    memcpy(directorio, (EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
    memcpy(memdatos, (EXT_DATOS *)&datosfich[4], MAX_BLOQUES_DATOS * SIZE_BLOQUE);

    while (1) {
        printf(">> ");
        fflush(stdin);
        fgets(comando, LONGITUD_COMANDO, stdin);
        comando[strcspn(comando, "\n")] = 0;

        sscanf(comando, "%s %s %s", orden, argumento1, argumento2);

        if (strcmp(orden, "info") == 0) {
            LeeSuperBloque(&ext_superblock);
        } else if (strcmp(orden, "bytemaps") == 0) {
            Printbytemaps(&ext_bytemaps);
        } else if (strcmp(orden, "dir") == 0) {
            Directorio(directorio, &ext_blq_inodos);
        } else if (strcmp(orden, "rename") == 0) {
            Renombrar(directorio, argumento1, argumento2);
        } else if (strcmp(orden, "imprimir") == 0) {
            Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
        } else if (strcmp(orden, "remove") == 0) {
            Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1);
        } else if (strcmp(orden, "copy") == 0) {
            Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2);
        } else if (strcmp(orden, "salir") == 0) {
            printf("Saliendo del programa...\n");
            fclose(fent);
            return 0;
        } else {
            printf("ERROR: Comando ilegal [bytemaps, copy, dir, info, imprimir, rename, remove, salir]\n");
        }
    }
}

//Cuando se copia y genera un archivo nuevo no cambia la información del súper bloque, pero sí la del bytemap
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup) {
    printf("Número total de inodos: %d\n", psup->s_inodes_count);
    printf("Número total de bloques: %d\n", psup->s_blocks_count);
    printf("Bloques libres: %d\n", psup->s_free_blocks_count);
    printf("Inodos libres: %d\n", psup->s_free_inodes_count);
    printf("Primer bloque de datos: %d\n", psup->s_first_data_block);
    printf("Tamaño del bloque: %d bytes\n", psup->s_block_size);
}

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps) {
    printf("Bytemap de bloques:\n");
    for (int i = 0; i < MAX_BLOQUES_PARTICION; i++) {
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\nBytemap de inodos:\n");
    for (int i = 0; i < MAX_INODOS; i++) {
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\n");
}

//Añadir tamaño y bloques
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos) {
    for (int i = 1; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo != NULL_INODO) {
            printf("Nombre: %s, Inodo: %d\n", directorio[i].dir_nfich, directorio[i].dir_inodo);
        }
    }
}

int Renombrar(EXT_ENTRADA_DIR *directorio, char *nombreantiguo, char *nombrenuevo) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombreantiguo) == 0) {
            strcpy(directorio[i].dir_nfich, nombrenuevo);
            return 0;
        }
    }
    printf("Error: Archivo no encontrado o nombre ya existente.\n");
    return -1;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            unsigned short int inodo_idx = directorio[i].dir_inodo;
            EXT_SIMPLE_INODE inodo = inodos->blq_inodos[inodo_idx];
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodo.i_nbloque[j] != NULL_BLOQUE) {
                    printf("%s", memdatos[inodo.i_nbloque[j]].dato);
                }
            }
            return 0;
        }
    }
    printf("Error: Archivo no encontrado.\n");
    return -1;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre) {
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
            unsigned short int inodo_idx = directorio[i].dir_inodo;
            EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[inodo_idx];
            inodo->size_fichero = 0;
            for (int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                if (inodo->i_nbloque[j] != NULL_BLOQUE) {
                    ext_bytemaps->bmap_bloques[inodo->i_nbloque[j]] = 0;
                    inodo->i_nbloque[j] = NULL_BLOQUE;
                }
            }
            ext_bytemaps->bmap_inodos[inodo_idx] = 0;
            strcpy(directorio[i].dir_nfich, "");
            directorio[i].dir_inodo = NULL_INODO;
            return 0;
        }
    }
    printf("Error: Archivo no encontrado.\n");
    return -1;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino) {
    int inodo_origen = -1;
    int inodo_destino = -1;

    // Buscar el archivo origen en el directorio
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (strcmp(directorio[i].dir_nfich, nombreorigen) == 0) {
            inodo_origen = directorio[i].dir_inodo;
            break;
        }
    }

    if (inodo_origen == -1) {
        printf("Error: Archivo origen no encontrado.\n");
        return -1;
    }

    // Buscar un inodo libre para el archivo destino
    for (int i = 0; i < MAX_INODOS; i++) {
        if (ext_bytemaps->bmap_inodos[i] == 0) {
            inodo_destino = i;
            ext_bytemaps->bmap_inodos[i] = 1; // Marcar como ocupado
            break;
        }
    }

    if (inodo_destino == -1) {
        printf("Error: No hay inodos libres disponibles.\n");
        return -1;
    }

    // Buscar una entrada libre en el directorio
    int entrada_libre = -1;
    for (int i = 0; i < MAX_FICHEROS; i++) {
        if (directorio[i].dir_inodo == NULL_INODO) {
            entrada_libre = i;
            break;
        }
    }

    if (entrada_libre == -1) {
        printf("Error: No hay entradas libres en el directorio.\n");
        return -1;
    }

    // Copiar metadatos del archivo origen al destino
    EXT_SIMPLE_INODE *inode_origen = &inodos->blq_inodos[inodo_origen];
    EXT_SIMPLE_INODE *inode_destino = &inodos->blq_inodos[inodo_destino];

    inode_destino->size_fichero = inode_origen->size_fichero;
    for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
        if (inode_origen->i_nbloque[i] != NULL_BLOQUE) {
            // Buscar un bloque libre
            for (int j = 0; j < MAX_BLOQUES_DATOS; j++) {
                if (ext_bytemaps->bmap_bloques[j] == 0) {
                    ext_bytemaps->bmap_bloques[j] = 1; // Marcar como ocupado
                    inode_destino->i_nbloque[i] = j;

                    // Copiar contenido del bloque origen al bloque destino
                    memcpy(&memdatos[j], &memdatos[inode_origen->i_nbloque[i]], SIZE_BLOQUE);
                    break;
                }
            }
        } else {
            inode_destino->i_nbloque[i] = NULL_BLOQUE;
        }
    }

    // Actualizar el directorio con el nuevo archivo
    strcpy(directorio[entrada_libre].dir_nfich, nombredestino);
    directorio[entrada_libre].dir_inodo = inodo_destino;

    printf("Archivo copiado con éxito.\n");
    return 0;
}
