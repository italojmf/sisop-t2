#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/t2fs.h"

int start = 0;
struct t2fs_superbloco superbloco;
unsigned char buffer[256];
// this will store the cluster of the currentDir/save the list also
unsigned int currentDir; // numero do cluster, simplificado, por ex: 2,3,4,5..
unsigned int fatherDir;

#define	MAX_FILE_NAME_SIZE	55
struct filePointer {
    char    name[MAX_FILE_NAME_SIZE]; 	/* Nome do arquivo. : string com caracteres ASCII (0x21 até 0x7A), case sensitive.             */
    int currentPointer;
};

typedef struct {
    struct t2fs_record file;
    int currentPointer;
} OpenedFiles;

OpenedFiles open[10];
int opened = 0;

/*-----------------------------------------------------------------------------
Função: Usada para identificar os desenvolvedores do T2FS.
	Essa função copia um string de identificação para o ponteiro indicado por "name".
	Essa cópia não pode exceder o tamanho do buffer, informado pelo parâmetro "size".
	O string deve ser formado apenas por caracteres ASCII (Valores entre 0x20 e 0x7A) e terminado por ‘\0’.
	O string deve conter o nome e número do cartão dos participantes do grupo.

Entra:	name -> buffer onde colocar o string de identificação.
	size -> tamanho do buffer "name" (número máximo de bytes a serem copiados).

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size){

}

void init() {
    if(start==0){
        read_sector(0,&superbloco);
        // Criacao diretorio raiz
        currentDir = superbloco.RootDirCluster;
        fatherDir = superbloco.RootDirCluster;
        // Verificar se ja tem informacoes escritas no root e escrever info do rootDir
        struct t2fs_record rootData[4];
        read_sector(superbloco.RootDirCluster*superbloco.SectorsPerCluster + superbloco.DataSectorStart,rootData);
        int ln = strcmp(rootData[0].name," ");
        if(ln == 0){
            int len = superbloco.SectorsPerCluster*4;
            struct t2fs_record list[len];
            struct t2fs_record rootDir;
            rootDir.bytesFileSize=superbloco.SectorsPerCluster*256;
            rootDir.firstCluster=superbloco.RootDirCluster;
            strcpy(rootDir.name,".");
            rootDir.TypeVal=0x02;
            struct t2fs_record rootFather;
            rootFather.bytesFileSize=superbloco.SectorsPerCluster*256;
            rootFather.firstCluster=superbloco.RootDirCluster;
            strcpy(rootFather.name,"..");
            rootFather.TypeVal=0x02;
            list[0] = rootDir;
            list[1] = rootFather;
            write_sector(superbloco.RootDirCluster*superbloco.SectorsPerCluster + superbloco.DataSectorStart,list);
        }
        start=1;
    }
}

void readFreeFAT(int c[], int sectors){
    int endFAT = superbloco.DataSectorStart;
    int currentFAT = superbloco.pFATSectorStart;
    int x=0;
    int sectorNum;
    while(currentFAT <= endFAT){
        sectorNum = readDisk(currentFAT);
        if(sectorNum >= 0){
            c[x] = currentFAT; // setor da fat onde tem espaco livre
            x++;
            if(x >= sectors){
                return;
            }
        }
        currentFAT+= 1;
    }
}

int readDisk(int fat){
    read_sector(fat,&buffer);
    int len = sizeof buffer/sizeof(int);
    int i, s=-1;
    for(i=0;i < len;i++){
        if(buffer[i] == 0){
            s=i;
            break;
        }
    }
    return s;
}

void clusterMap(int setorFAT[],int map[]){
    int startDados = superbloco.DataSectorStart;
    int sector = superbloco.SectorsPerCluster;
    int i,s;
    read_sector(setorFAT[0],buffer);
    for(i=0;i<64;i++){
        if(buffer[i] == 0){
            s=i;
            break;
        }
    }
    map[0] = startDados + ((((setorFAT[0] - 1)*64) + s) * sector);
}

void readFileName(char * filename, char nome[]){

    char len = strlen(filename);
    char auxName[len+1];
    char *aux;
    strcpy(auxName,filename);
    if(auxName[1] == '.' && auxName[0] == '.'){
        aux = strtok(auxName, "..");
        aux = strtok(aux,"/");
    }
    else if(auxName[0] == '.'){
        aux = strtok(auxName, ".");
        aux = strtok(aux,"/");
    }
    else { aux = strtok(auxName,"/"); }

    while(aux!=NULL && strcmp(aux," ")!=0){
        strcpy(auxName,aux);
        aux = strtok(NULL,"/");
        if(aux != NULL){
            strcpy(auxName,aux);
        }
        else{
            strcpy(nome,auxName);
            break;
        }
    }
    strcpy(nome,auxName);
}

int checkPath(char path[]){
    int offset = -1;
    int len = strlen(path);
    char auxPath[len];
    char *dpath;
    struct t2fs_record ret[superbloco.SectorsPerCluster];
    int k,i;
    strcpy(auxPath,path);
    int newoffset = 0;
    int troca=1;

    if(path[0] == '/'){
        dpath = strtok(auxPath, "/");
        offset = superbloco.RootDirCluster*superbloco.SectorsPerCluster;
    }
    else if(path[0] == '.' && path[1] == '.'){
        dpath = strtok(auxPath, "..");
        offset =  fatherDir*superbloco.SectorsPerCluster;
        dpath = strtok(dpath, "/");
    }
    else if(path[0] == '.'){
        dpath = strtok(auxPath, ".");
        offset = currentDir*superbloco.SectorsPerCluster;
        dpath = strtok(dpath, "/");
    }
    else return offset = currentDir*superbloco.SectorsPerCluster;

    while(dpath != NULL && strcmp(dpath," ")!=0 && troca){
        newoffset=offset;
        for(i=0;i<superbloco.SectorsPerCluster;i++){ //
            read_sector(offset + superbloco.DataSectorStart + i,ret); // Deve haver uma lista em "buffer" contendo as entradas de diretorio
            // busca pro subdiretorios
            for(k=0;k<4;k++){ // ALTERAR O 4 POR SECTOR/CLUSTER / SIZE DE UM RECORD
                if(dpath != NULL && strcmp(dpath, ret[k].name) == 0 && ret[k].TypeVal == 0x02){
                    offset = ret[k].firstCluster*superbloco.SectorsPerCluster;
                }
            }
        }
        if(newoffset == offset){
            troca = 0;
        }
        dpath = strtok(NULL,"/");
    }
    if(troca == 0)
        return -1;
    return offset;
}

int writeToList(struct t2fs_record rec, int sector){
    struct t2fs_record list[4];
    struct t2fs_record listar[4];
    int ln=4;
    int i;
    for(i=0;i < superbloco.SectorsPerCluster;i++){
        read_sector(sector+i,list);

        if(strcmp(list[0].name,"") == 0)
            ln = 0;
        else if(strcmp(list[1].name,"") == 0)
            ln = 1;
        else if(strcmp(list[2].name,"") == 0)
            ln = 2;
        else if(strcmp(list[3].name,"") == 0)
            ln = 3;
        if(ln < 4){
            //pode escrever
            list[ln] = rec;
            write_sector(sector+i,list);
            return 1;
        }
    }
    return -1;
}

void findPath(char *filepath, char filename[], char path[]){
    // TODO: IF FILENAME AND A PART OF THE PATH ARE EQUAL, STRTOK WILL BREAK BEFORE PROPER PATH (E.G: /xd/xd/ -> /)
    char len = strlen(filepath);
    char auxPath[len+1];
    char *aux;
    char *name;
    strcpy(auxPath,filepath);

    aux = strstr(auxPath,filename);
    strncpy(aux," ",sizeof filename/sizeof(char));

    strcpy(path,auxPath);
}

int sectorToWrite(char path[]){
    int offset = -1;
    int len = strlen(path);
    char auxPath[len];
    char *dpath;
    struct t2fs_record ret[superbloco.SectorsPerCluster];
    int k,i;
    strcpy(auxPath,path);
    int newoffset = 0;
    int troca=1;

    if(path[0] == '/'){
        dpath = strtok(auxPath, "/");
        offset = superbloco.RootDirCluster*superbloco.SectorsPerCluster;
    }
    else if(path[0] == '.' && path[1] == '.'){
        dpath = strtok(auxPath, "..");
        offset =  fatherDir*superbloco.SectorsPerCluster;
        dpath = strtok(dpath, "/");
    }
    else if(path[0] == '.'){
        dpath = strtok(auxPath, ".");
        offset = currentDir*superbloco.SectorsPerCluster;
        dpath = strtok(dpath, "/");
    }
    else return offset = currentDir*superbloco.SectorsPerCluster + superbloco.DataSectorStart;

    while(dpath != NULL && strcmp(dpath," ")!=0 && troca){
        newoffset=offset;
        for(i=0;i<superbloco.SectorsPerCluster;i++){ //
            read_sector(offset + superbloco.DataSectorStart + i,ret); // Deve haver uma lista em "buffer" contendo as entradas de diretorio
            // busca pro subdiretorios
            for(k=0;k<4;k++){ // ALTERAR O 4 POR SECTOR/CLUSTER / SIZE DE UM RECORD
                if(dpath != NULL && strcmp(dpath, ret[k].name) == 0 && ret[k].TypeVal == 0x02){
                    //puts(ret[k].name);
                    //printf("%d, %x\n",ret[k].firstCluster,ret[k].firstCluster);
                    offset = ret[k].firstCluster*superbloco.SectorsPerCluster;
                }
            }
        }
        if(newoffset == offset){
            troca = 0;
        }
        dpath = strtok(NULL,"/");
    }
    return offset + superbloco.DataSectorStart;
}

int addFileToList(struct t2fs_record rec, char *filepath, char filename[]){
    int len = strlen(filepath);
    char path[len];
    findPath(filepath, filename, path);
    int sector = -1;
    sector = sectorToWrite(path);
    if(sector != -1){
        return writeToList(rec, sector);
    }
    else return -1;
}

void writeFATMapping(int setorFAT[], int map[]){
    int endFAT = superbloco.DataSectorStart;
    int currentFAT = superbloco.pFATSectorStart;
    currentFAT += 2;
    int i,j=1;
    read_sector(setorFAT[0],buffer);
    for(i=0;i<64;i++){
        if(buffer[i] == 0){
            buffer[i] = map[j];
            write_sector(setorFAT[0],buffer);
            j++;
            break;
        }
    }
}

void clearFat(int sector, int pos){
    read_sector(sector,&buffer);
    buffer[pos] = 0x00;
    write_sector(sector,buffer);
}

void findFileTrace(int cluster){
    int i; // > recuperar posicao
    int j; // recuperar setor 
    int curr = cluster;
    do {
        if(curr > 64){
            i = curr%64;
            j = curr/64;
        }
        else {
            i=curr;
            j=0;
        }
        read_sector(j+superbloco.pFATSectorStart,&buffer);
        curr = buffer[i];
        clearFat(j+1,i);
    } while(curr != 0xFF);
}

int isFileOpen(struct t2fs_record new){
    int i;
    for(i = 0; i < 10; ++i)
    {
        if(new.firstCluster == open[i].file.firstCluster)
            return 1;
    }
    return -1;
}

FILE2 create2 (char *filename){
    init();
    unsigned int setorFAT[1] = {0}; // POSICAO DO SETOR DA FAT COM ESPACO LIVRE
    unsigned int map[2] = {0}; // POSICAO NA AREA DE DADOS DO CLUSTER LIDO EM CLUSTER[]
    int addFile;

    readFreeFAT(setorFAT,1);
    clusterMap(setorFAT, map);
    map[1] = 0xFFFFFFFF;
    char nome[MAX_FILE_NAME_SIZE], path[256];
    readFileName(filename,nome);
    findPath(filename, nome, path);

    if(checkPath(path) >= 0){
        struct t2fs_record newFile;
        newFile.bytesFileSize=0;
        newFile.firstCluster=(map[0] - superbloco.DataSectorStart)/superbloco.SectorsPerCluster;
        newFile.TypeVal = 0x01;
        strcpy(newFile.name, nome);
        addFile = addFileToList(newFile, filename, nome); // Adiciona as infos do arquivo ao seu diretorio
        if(addFile < 0){
            return -1;
        }
        writeFATMapping(setorFAT,map); // Basicamente ler o valor do map e escrever o proximo valor nele na area da FAT => FAT[MAPPING[N]] = MAPPING[N+1]
        return newFile.firstCluster; // TBD
    }
    else return -1;
        /** - Atualizar estrutura de entrada de diretorio
            > byteSize = 0
            > firstCluster = mapping a ser feito
            > name = pega do filename /x/y/name
            > tipo = arquivo
        - Ignora currentPointer
        - Verificar se path eh unico e valido
        - Verificar cluster livre e inserir arquivo (num de clusters necessarios = (tamanho do arquivo/256)/setores por cluster)
        - Salvar setores usados
        - Salvar os setores do arquivo na FAT
        - Retorna identificador **/
}

int delete2 (char *filename){
    init();
    struct t2fs_record list[4];
    int i,j,s = -1;
    int cluster;
    char len = strlen(filename);
    char auxName[len+1];
    static const struct t2fs_record deleted;

    strcpy(auxName,filename);

    for(i=0;i<4;i++){
        read_sector(currentDir*superbloco.SectorsPerCluster + superbloco.DataSectorStart + i,list);
        for(j=0;j<4;j++){
            if(strcmp(list[i].name, auxName) == 0 && list[i].TypeVal == 0x01){
                cluster = list[i].firstCluster;
                list[i] = deleted;
                s=i;
                break;
            }
        }
        if(s==i)
            break;
    }
    if(s!=i)
        return -1;
    write_sector(currentDir*superbloco.SectorsPerCluster + superbloco.DataSectorStart + s,list);
    findFileTrace(cluster);
}

FILE2 open2 (char *filename){
    init();
    int len = strlen(filename);
    char auxName[len];
    strcpy(auxName, filename);
    OpenedFiles new;
    struct t2fs_record list[4];
    int i,j, s=-1;

    for(i=0;i<4;i++){
        read_sector(currentDir*superbloco.SectorsPerCluster + superbloco.DataSectorStart + i,list);
        for(j=0;j<4;j++){
            if(strcmp(list[j].name, auxName) == 0 && list[j].TypeVal == 0x01){
                new.file = list[j];
                new.currentPointer = 0;
                s=i;                
                break;
            }
        }
        if(s==i)
            break;
    }
    if(s!=i)
        return -1;
    if(isFileOpen(new.file) >0)
        return -1;
    open[opened] = new;
    opened++;
    return new.file.firstCluster;
}

int close2 (FILE2 handle){
    init();
    static const OpenedFiles holder;
    int i;
    for ( i = 0; i < 10; ++i)
    {
        if(open[i].file.firstCluster == handle){
            open[i] = holder;
            return 0;
        }
    }
    return -1;
}

int read2 (FILE2 handle, char *buffer, int size){
    init();
    int cluster = handle;
    char data[256];
    OpenedFiles curr;
    int i,len;
    for (i = 0; i < 10; ++i)
    {
        if(open[i].file.firstCluster == cluster){
            curr = open[i];
            break;
        }
        if(i==9){
            return -1;
        }
    }
    read_sector(handle*superbloco.SectorsPerCluster + superbloco.DataSectorStart, &data);
    char dt[size];
    strncpy(dt,data,size);
    strcpy(buffer,dt);
    
    return strlen(buffer);
}

int write2 (FILE2 handle, char *buffer, int size){
    init();
    int cluster = handle;
    int bLen = strlen(buffer);
    int byteSec = 256;
    int sects = size/byteSec; // qtde de setores que precisa usar pra escrever 
    OpenedFiles curr;
    int i;
    char data[256];
    int len;

    for (i = 0; i < 19; ++i)
    {
        if(open[i].file.firstCluster == handle){
            curr = open[i];
            break;
        }

    }
    read_sector(handle*superbloco.SectorsPerCluster + superbloco.DataSectorStart, &data);
    len = strlen(data);
    char dt[len];
    strcpy(dt,data);
/*    if(len == 256)
        //precisa ler proximo setor
    else if(len + size > 256) // se isso acontecer e estivar no ultimo setor -> erro
        //insere uma parte no setor atual e outra no proximo
    else if(len + size < 256)
        //insere no setor atual a info*/

    exit(0);
}

int truncate2 (FILE2 handle){}

int seek2 (FILE2 handle, unsigned int offset){}

int mkdir2 (char *pathname){
    init();
    unsigned int setorFAT[1] = {0}; // POSICAO DO SETOR DA FAT COM ESPACO LIVRE
    unsigned int map[2] = {0}; // POSICAO NA AREA DE DADOS DO CLUSTER LIDO EM CLUSTER[]
    char nome[MAX_FILE_NAME_SIZE], path[256];

    readFreeFAT(setorFAT,1);
    clusterMap(setorFAT, map);
    map[1] = 0xFFFFFFFF;

    readFileName(pathname,nome);
    findPath(pathname, nome, path);

    if(checkPath(path) >= 0){
        int spc = superbloco.SectorsPerCluster;
        int start = superbloco.DataSectorStart;
        struct t2fs_record list[2];
        struct t2fs_record newDir;
        newDir.bytesFileSize=0;
        newDir.firstCluster= (map[0] - start)/spc;
        strcpy(newDir.name,nome);
        newDir.TypeVal=0x02;

        struct t2fs_record current;
        current.bytesFileSize=0;
        current.firstCluster = (map[0] - start)/spc;
        strcpy(current.name,".");
        current.TypeVal=0x02;

        struct t2fs_record father;
        father.bytesFileSize=0;
        father.firstCluster = (sectorToWrite(path) - start)/spc;
        strcpy(father.name,"..");
        father.TypeVal=0x02;

        list[0] = current;
        list[1] = father;
        if(writeToList(newDir, sectorToWrite(path)) != -1){
            write_sector(newDir.firstCluster*spc + start,list);
            writeFATMapping(setorFAT,map); // Basicamente ler o valor do map e escrever o proximo valor nele na area da FAT => FAT[MAPPING[N]] = MAPPING[N+1]
            return 0;
        }
        else return -1;
    }
    return -1;
}

int rmdir2 (char *pathname){}

int chdir2 (char *pathname){}

int getcwd2 (char *pathname, int size){}

DIR2 opendir2 (char *pathname){}

int readdir2 (DIR2 handle, DIRENT2 *dentry){}

int closedir2 (DIR2 handle){}

