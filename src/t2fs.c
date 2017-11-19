#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/t2fs.h"

int start = 0;
struct t2fs_superbloco superbloco;
unsigned char buffer[256];
// this will store the cluster of the currentDir/save the list also
int currentDir;

#define	MAX_FILE_NAME_SIZE	55
struct filePointer {
    char    name[MAX_FILE_NAME_SIZE]; 	/* Nome do arquivo. : string com caracteres ASCII (0x21 até 0x7A), case sensitive.             */
    int currentPointer;
};

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
        // Verificar se ja tem informacoes escritas no root e escrever info do rootDir
        struct t2fs_record rootData[4];
        read_sector(superbloco.RootDirCluster*superbloco.SectorsPerCluster + superbloco.DataSectorStart,rootData);
        int ln = sizeof(rootData)/sizeof(struct t2fs_record);
        if(ln < 1){
            int len = superbloco.SectorsPerCluster*4;
            struct t2fs_record list[len];
            struct t2fs_record rootDir;
            rootDir.bytesFileSize=superbloco.SectorsPerCluster*256;
            rootDir.firstCluster=superbloco.RootDirCluster;
            strcpy(rootDir.name,"root");
            rootDir.TypeVal=0x02;
            list[0] = rootDir;
            write_sector(superbloco.RootDirCluster*superbloco.SectorsPerCluster + superbloco.DataSectorStart,list);
        }
        start=1;
    }
}

void readFreeFAT(int c[], int clusters){
    int endFAT = superbloco.DataSectorStart;
    int currentFAT = superbloco.pFATSectorStart;
    currentFAT += 2;
    int x=0;
    int clustNum;
    while(currentFAT <= endFAT){
        clustNum = readDisk(currentFAT);
        if(clustNum == 0){
            c[x] = currentFAT; // currentFAT apota pra um cluster
            x++;
            if(x >= clusters){
                return;
            }
        }
        currentFAT+= 1;
    }
}

int readDisk(int fat){
    read_sector(fat,&buffer);

    int sect = *((WORD *)(buffer));
    return sect;
}

// Return to MAP the free sector read from fat as sector number
void clusterMap(int cluster[],int map[]){
    int startDados = superbloco.DataSectorStart;
    int sector = superbloco.SectorsPerCluster;
    int i;
    for(i=0; i < sizeof cluster; i++){
        map[i] = cluster[i]*sector + startDados;
    }
}

void readFileName(char * filename, char nome[]){

    char len = strlen(filename);
    char auxName[len+1];
    char *aux;
    strcpy(auxName,filename);
    aux = strtok(auxName,"/");
    while(aux!=NULL){
        strcpy(auxName,aux);
        aux = strtok(NULL,"/");
        if(aux != NULL){
            strcpy(auxName,aux);
        }
        else{
            strcpy(nome,auxName);
        }
    }
}

int checkPath(char *filename){
    return 1;
}

void writeToList(struct t2fs_record rec, int sector){
    struct t2fs_record list[4];
    struct t2fs_record listar[4];
    int i;
    for(i=0;i<superbloco.SectorsPerCluster;i++){
        read_sector(sector+i,list);
        int ln = sizeof(list)/sizeof(struct t2fs_record); // PORQUE SEMPRE RETORNA 4??
        if(ln < 4){
            //pode escrever
            list[ln] = rec;
            write_sector(sector+i,list);
            read_sector(sector+i,listar);
            printf("%s, %d", listar[ln].name,listar[ln].firstCluster);
            fflush(stdout);
            return;
        }
    }
}

void findPath(char *filepath, char filename[], char path[]){
    // TODO: IF FILENAME AND A PART OF THE PATH ARE EQUAL, STRTOK WILL BREAK BEFORE PROPER PATH (E.G: /xd/xd/ -> /)
    char len = strlen(filepath);
    char auxPath[len+1];
    char *aux;
    strcpy(auxPath,filepath);
    aux=strtok(auxPath,filename);
    if(aux != NULL && strcmp(aux,"/")!=0)
        strcpy(path,aux);
    else strcpy(path,"root");
}

int sectorToWrite(char path[]){ // A SER TESTADO (ELSE)

    if(strcmp(path,"root")==0){
        return superbloco.RootDirCluster*superbloco.SectorsPerCluster + superbloco.DataSectorStart;
    }
    else{
        int len = strlen(path);
        char auxPath[len];
        char *dpath;
        struct t2fs_record ret[superbloco.SectorsPerCluster];
        int k,i;
        strcpy(auxPath,path);
        dpath = strtok(auxPath,"/");
        int offset = superbloco.RootDirCluster*superbloco.SectorsPerCluster;
        int newoffset = 0;
        int troca=1;
        while(dpath!=NULL && troca){/** Essa busca é bem parecida com a busca q vai ser feita no "checkPath", só pra constar**/
            dpath = strtok(NULL,"/");
            newoffset=offset;
            for(i=0;i<superbloco.SectorsPerCluster;i++){ //
                read_sector(offset + superbloco.DataSectorStart+i,ret); // Deve haver uma lista em "buffer" contendo as entradas de diretorio
                for(k=0;k<4;k++){ // ALTERAR O 4 POR SECTOR/CLUSTER / SIZE DE UM RECORD
                    if(dpath != NULL && strcmp(dpath, ret[k].name) == 0){ // existe diretorio esperado na lista (provavelmente vai precisar de um FOR aqui pra verificar toda lista)
                        offset = ret[k].firstCluster*superbloco.SectorsPerCluster;
                    }
                }
            }
            if(newoffset == offset)
                troca = 0;
        }
        if(offset != superbloco.RootDirCluster*superbloco.SectorsPerCluster)
            return offset + superbloco.DataSectorStart;
        else return -1;
    }

}

void addFileToList(struct t2fs_record rec, char *filepath, char filename[]){
    int len = strlen(filepath);
    char path[len];
    findPath(filepath, filename, path);
    int sector = sectorToWrite(path);
    if(free){
        writeToList(rec, sector);
    }
}

/*-----------------------------------------------------------------------------
Função: Criar um novo arquivo.
	O nome desse novo arquivo é aquele informado pelo parâmetro "filename".
	O contador de posição do arquivo (current pointer) deve ser colocado na posição zero.
	Caso já exista um arquivo ou diretório com o mesmo nome, a função deverá retornar um erro de criação.
	A função deve retornar o identificador (handle) do arquivo.
	Esse handle será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do arquivo criado.

Entra:	filename -> path absoluto para o arquivo a ser criado. Todo o "path" deve existir.

Saída:	Se a operação foi realizada com sucesso, a função retorna o handle do arquivo (número positivo).
	Em caso de erro, deve ser retornado um valor negativo.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename){
    init();
    int cluster[1] = {0};
    int map[1] = {0};
    int fileHandler;

    readFreeFAT(cluster,1);
    clusterMap(cluster, map);

    if(checkPath(filename)){
        struct t2fs_record newFile;
        newFile.bytesFileSize=0;
        newFile.firstCluster=map[0];
        newFile.TypeVal = 0x01;
        char nome[MAX_FILE_NAME_SIZE];
        readFileName(filename,nome);
        strcpy(newFile.name, nome);

        addFileToList(newFile, filename, nome); // Adiciona as infos do arquivo ao seu diretorio
        //writeFATMapping(map); // Basicamente ler o valor do map e escrever o proximo valor nele na area da FAT => FAT[MAPPING[N]] = MAPPING[N+1]
        return fileHandler; // TBD
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
/*-----------------------------------------------------------------------------
Função:	Apagar um arquivo do disco.
	O nome do arquivo a ser apagado é aquele informado pelo parâmetro "filename".

Entra:	filename -> nome do arquivo a ser apagado.

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int delete2 (char *filename){}


/*-----------------------------------------------------------------------------
Função:	Abre um arquivo existente no disco.
	O nome desse novo arquivo é aquele informado pelo parâmetro "filename".
	Ao abrir um arquivo, o contador de posição do arquivo (current pointer) deve ser colocado na posição zero.
	A função deve retornar o identificador (handle) do arquivo.
	Esse handle será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do arquivo criado.
	Todos os arquivos abertos por esta chamada são abertos em leitura e em escrita.
	O ponto em que a leitura, ou escrita, será realizada é fornecido pelo valor current_pointer (ver função seek2).

Entra:	filename -> nome do arquivo a ser apagado.

Saída:	Se a operação foi realizada com sucesso, a função retorna o handle do arquivo (número positivo)
	Em caso de erro, deve ser retornado um valor negativo
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename){}


/*-----------------------------------------------------------------------------
Função:	Fecha o arquivo identificado pelo parâmetro "handle".

Entra:	handle -> identificador do arquivo a ser fechado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle){}


/*-----------------------------------------------------------------------------
Função:	Realiza a leitura de "size" bytes do arquivo identificado por "handle".
	Os bytes lidos são colocados na área apontada por "buffer".
	Após a leitura, o contador de posição (current pointer) deve ser ajustado para o byte seguinte ao último lido.

Entra:	handle -> identificador do arquivo a ser lido
	buffer -> buffer onde colocar os bytes lidos do arquivo
	size -> número de bytes a serem lidos

Saída:	Se a operação foi realizada com sucesso, a função retorna o número de bytes lidos.
	Se o valor retornado for menor do que "size", então o contador de posição atingiu o final do arquivo.
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size){}


/*-----------------------------------------------------------------------------
Função:	Realiza a escrita de "size" bytes no arquivo identificado por "handle".
	Os bytes a serem escritos estão na área apontada por "buffer".
	Após a escrita, o contador de posição (current pointer) deve ser ajustado para o byte seguinte ao último escrito.

Entra:	handle -> identificador do arquivo a ser escrito
	buffer -> buffer de onde pegar os bytes a serem escritos no arquivo
	size -> número de bytes a serem escritos

Saída:	Se a operação foi realizada com sucesso, a função retorna o número de bytes efetivamente escritos.
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size){}


/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo.
	Remove do arquivo todos os bytes a partir da posição atual do contador de posição (CP)
	Todos os bytes a partir da posição CP (inclusive) serão removidos do arquivo.
	Após a operação, o arquivo deverá contar com CP bytes e o ponteiro estará no final do arquivo

Entra:	handle -> identificador do arquivo a ser truncado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle){}


/*-----------------------------------------------------------------------------
Função:	Reposiciona o contador de posições (current pointer) do arquivo identificado por "handle".
	A nova posição é determinada pelo parâmetro "offset".
	O parâmetro "offset" corresponde ao deslocamento, em bytes, contados a partir do início do arquivo.
	Se o valor de "offset" for "-1", o current_pointer deverá ser posicionado no byte seguinte ao final do arquivo,
		Isso é útil para permitir que novos dados sejam adicionados no final de um arquivo já existente.

Entra:	handle -> identificador do arquivo a ser escrito
	offset -> deslocamento, em bytes, onde posicionar o "current pointer".

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, unsigned int offset){}


/*-----------------------------------------------------------------------------
Função:	Criar um novo diretório.
	O caminho desse novo diretório é aquele informado pelo parâmetro "pathname".
		O caminho pode ser ser absoluto ou relativo.
	São considerados erros de criação quaisquer situações em que o diretório não possa ser criado.
		Isso inclui a existência de um arquivo ou diretório com o mesmo "pathname".

Entra:	pathname -> caminho do diretório a ser criado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname){}


/*-----------------------------------------------------------------------------
Função:	Apagar um subdiretório do disco.
	O caminho do diretório a ser apagado é aquele informado pelo parâmetro "pathname".
	São considerados erros quaisquer situações que impeçam a operação.
		Isso inclui:
			(a) o diretório a ser removido não está vazio;
			(b) "pathname" não existente;
			(c) algum dos componentes do "pathname" não existe (caminho inválido);
			(d) o "pathname" indicado não é um arquivo;

Entra:	pathname -> caminho do diretório a ser criado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname){}


/*-----------------------------------------------------------------------------
Função:	Altera o current path
	O novo caminho do diretório a ser usado como current path é aquele informado pelo parâmetro "pathname".
	São considerados erros quaisquer situações que impeçam a operação.
		Isso inclui:
			(a) "pathname" não existente;
			(b) algum dos componentes do "pathname" não existe (caminho inválido);
			(c) o "pathname" indicado não é um diretório;

Entra:	pathname -> caminho do diretório a ser criado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname){}


/*-----------------------------------------------------------------------------
Função:	Função que informa o diretório atual de trabalho.
	O T2FS deve copiar o pathname do diretório de trabalho, incluindo o ‘\0’ do final do string, para o buffer indicado por "pathname".
	Essa cópia não pode exceder o tamanho do buffer, informado pelo parâmetro "size".

Entra:	pathname -> ponteiro para buffer onde copiar o pathname
	size -> tamanho do buffer "pathname" (número máximo de bytes a serem copiados).

Saída:
	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Caso ocorra algum erro, a função retorna um valor diferente de zero.
	São considerados erros quaisquer situações que impeçam a cópia do pathname do diretório de trabalho para "pathname",
	incluindo espaço insuficiente, conforme informado por "size".
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size){}


/*-----------------------------------------------------------------------------
Função:	Abre um diretório existente no disco.
	O caminho desse diretório é aquele informado pelo parâmetro "pathname".
	Se a operação foi realizada com sucesso, a função:
		(a) deve retornar o identificador (handle) do diretório
		(b) deve posicionar o ponteiro de entradas (current entry) na primeira posição válida do diretório "pathname".
	O handle retornado será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do diretório.

Entra:	pathname -> caminho do diretório a ser aberto

Saída:	Se a operação foi realizada com sucesso, a função retorna o identificador do diretório (handle).
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname){}


/*-----------------------------------------------------------------------------
Função:	Realiza a leitura das entradas do diretório identificado por "handle".
	A cada chamada da função é lida a entrada seguinte do diretório representado pelo identificador "handle".
	Algumas das informações dessas entradas devem ser colocadas no parâmetro "dentry".
	Após realizada a leitura de uma entrada, o ponteiro de entradas (current entry) é ajustado para a próxima entrada válida
	São considerados erros:
		(a) término das entradas válidas do diretório identificado por "handle".
		(b) qualquer situação que impeça a realização da operação

Entra:	handle -> identificador do diretório cujas entradas deseja-se ler.
	dentry -> estrutura de dados onde a função coloca as informações da entrada lida.

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor negativo
		Se o diretório chegou ao final, retorna "-END_OF_DIR" (-1)
		Outros erros, será retornado um outro valor negativo
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry){}


/*-----------------------------------------------------------------------------
Função:	Fecha o diretório identificado pelo parâmetro "handle".

Entra:	handle -> identificador do diretório que se deseja fechar (encerrar a operação).

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle){}

