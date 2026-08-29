#include <fstream>
#include <iostream>
#include <format>
#include <string>
#include <vector>
#include <cstdint>



void readBinaryFile(const std::string& fileName){
    //Usamos std::ios::ate para poder mover el puntero libremente
    //Util tambien para poder comprobar el size del archivo
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        std::cerr << "NO SE PUDO ABRIR EL ARCHIVO"<<std::endl;
        return;
    }

    //Obtenemos dimensiones del archivo extraido
    std::streamsize size = file.tellg(); //aqui el puntero esta al final
    file.seekg(0, std::ios::beg); //me muevo al inicio

    //obtengo el valor final donde esta el puntero que me dice el tamanio
    //Y me da una idea del size o cantidad todal de bytes que maneja el archivo.
    std::vector<uint8_t> buffer(size);
    //Avanzamos ya no por sizeof, si no por medio de un offset
    
    //deserializamos la informacion
    if(!file.read(reinterpret_cast<char*>(buffer.data()), size)){
        std::cerr <<"ERROR: NO SE PUDO"<<std::endl;
    }
    
    //Variable que maneja mi puntero
    size_t offset =0;

    //Verificacion de BOM
    if(buffer.size()>=3 && buffer[0]== 0xEF 
        && buffer[1] == 0xBB & buffer[2] == 0xBF ){
            //BOM detectado
            std::cout<< "FLAG: BOM DETECTADO, CAMBIANDO OFFSET"<<std::endl;
            offset=3;
    }else{
        std::cout<<"FLAG: EL ARCHIVO NO CONTIENE BOM"<<std::endl;
    }

    



}

//Se le ponen argumentos al main para poder capturar la informacion desde la consola
int main(int argc, char* argv[]){

    //Validacion de ingresado ruta de archivo
    if(argc <2){
        std::cerr << "No se ingreso un ruta de archivo"<<std::endl;
        return 1;
    }

    readBinaryFile(argv[1]);

    return 0;
}