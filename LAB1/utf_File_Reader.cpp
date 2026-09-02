#include <fstream>
#include <iostream>
#include <format>
#include <string>
#include <vector>
#include <cstdint>




struct errorUTF8{
    size_t offset;
    std::string error_message;
};



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
        std::cerr <<"ERROR: NO SE PUDO LEER EL ARCHIVO"<<std::endl;
    }
    
    //Variable que maneja mi puntero
    size_t offset =0;

    //Verificacion de BOM
    if(buffer.size()>=3 && buffer[0]== 0xEF 
        && buffer[1] == 0xBB && buffer[2] == 0xBF ){
            //BOM detectado
            offset=3;
    }
    
    //Variables controladores de datos
    std::vector<uint32_t> code_points;
    //1 byte - 2 bytes - 3 bytes - 4 bytes
    int bytes_controller[4]= {0,0,0,0};
    std::vector<errorUTF8> reportar_error;



    /*
        Cosas que se hacen independientemente este bueno o no el byte
        -guardar su registro 
        -crear el codepoint
        -cambiar offset
    
    
    */


    //Puntero se mantenga dentro de todos los bytes del contenido
    while(offset < buffer.size()){
        //Extraer el byte en que actualmente estamos    
        uint8_t b1 = buffer[offset];
        uint32_t code_point=0x00;

        if((b1 & 0x80) == 0x00){
            //solo 1 byte
            code_point =b1;
            code_points.push_back(code_point);
            bytes_controller[0]++;
            offset+=1;

        }else if((b1 & 0xE0) == 0xC0){
            //2 bytes
            
            //Validacion de secuencia incompleta con b1
            if(offset +1 >= buffer.size()){
                errorUTF8 errortmp;
                errortmp.offset=offset;
                errortmp.error_message="Secuencia incompleta";
                reportar_error.push_back(errortmp);
                offset+=1;
                continue;
            }else{

                //obtencion de b2
                uint8_t b2 = buffer[offset+1];

                //validacion de byte de secuencia no encontrado
                if((b2 & 0xC0) != 0x80){
                    errorUTF8 errortmp2;
                    errortmp2.offset=offset;
                    errortmp2.error_message= "Byte de continuacion esperado, no encontrado";
                    reportar_error.push_back(errortmp2);
                    offset+= 1;
                    continue;
                }
                code_point= ((b1 & 0x1F) << 6) | (b2 & 0x3F);
                bytes_controller[1]++;
                offset = offset + 2;
                code_points.push_back(code_point);
                
            }
            

        }else if((b1 & 0xF0) == 0xE0){
            //3 bytes
            

            //Validar secuencia incompleta con b1
            if(offset +2 >= buffer.size()){
                errorUTF8 errortmp;
                errortmp.offset=offset;
                errortmp.error_message="Secuencia incompleta";
                reportar_error.push_back(errortmp);
                offset += 1;
                continue;
            }else{
                uint8_t b2 = buffer[offset+ 1];
                uint8_t b3 = buffer[offset +2];

                //Validacion de byte de secuencia no encontrado
                if((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80){
                    errorUTF8 errortmp2;
                    errortmp2.offset=offset;
                    errortmp2.error_message= "Byte de continuacion esperado, no encontrado";
                    reportar_error.push_back(errortmp2);
                    offset= offset +1;
                    continue;
                } //creo que aqui deberia ir un else que si permita construir el codepoint

                code_point = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
                bytes_controller[2]++;
                offset = offset +3;
                code_points.push_back(code_point);
            }

           
            

        }else if((b1 & 0xF8) ==  0xF0){
            //4 bytes
            
            if(offset+3 >= buffer.size()){
                errorUTF8 errortmp;
                errortmp.offset=offset;
                errortmp.error_message="Secuencia incompleta";
                reportar_error.push_back(errortmp);
                offset += 1;
                continue;
            }else{
                uint8_t b2 = buffer[offset+1];
                uint8_t b3 = buffer[offset+2];
                uint8_t b4 = buffer[offset+3];

                //Validacion de byte de secuencia no encontrado
                if((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80 || (b4 & 0xC0) !=0x80){
                    errorUTF8 errortmp2;
                    errortmp2.offset=offset;
                    errortmp2.error_message= "Byte de continuacion esperado, no encontrado";
                    reportar_error.push_back(errortmp2);
                    offset= offset +1;
                    continue;
                } 

                code_point = ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) << 6) | (b4 & 0x3F) ;
                bytes_controller[3]++;
                 offset = offset +4;
                code_points.push_back(code_point);
            }

        }else if((b1 & 0xC0) == 0x80){
            //Byte Huerfano
            errorUTF8 error;
            error.offset=offset;
            error.error_message="byte de continuacion inesperado";
            reportar_error.push_back(error);
            offset+=1;
            continue;
        }else{
            //Caso de Bytes lideres invalidos
            errorUTF8 error;
            error.offset=offset;
            error.error_message="Byte lider invalido";
            reportar_error.push_back(error);
            offset+=1;
            continue;
        }

    }

    //Decodificacion de caracteres
    std::cout << "=== Contenido Decodificado ==="<< std::endl;
    for(uint32_t cp : code_points){
        if(cp >= 0x20 && cp <= 0x7E ){
            std::cout << static_cast<char>(cp) <<std::endl;
        }else{
            std::cout<< std::format("U+{:04X}", cp)<< std::endl;
        }
    }

    //Impresion de errores
    std::cout<< "=== Errores Detectados ==="<<std::endl;
    for(errorUTF8 err : reportar_error){
        std::cout << std::format("[offset {}] {}",err.offset, err.error_message)<<std::endl;
    }

    //Impresion de resumen
    std::cout   << "=== Resumen ==="<< std::endl;
    std::cout << std::format("Bytes totales:        {}", buffer.size())<<std::endl;
    std::cout << std::format("Code points validos:      {}", code_points.size())<<std::endl;
    std::cout << std::format("  - 1 byte:       {}", bytes_controller[0])<<std::endl;
    std::cout << std::format("   - 2 bytes:      {}", bytes_controller[1])<<std::endl;
    std::cout << std::format("  - 3 bytes:      {}", bytes_controller[2])<<std::endl;
    std::cout << std::format("  - 4 bytes:      {}", bytes_controller[3])<<std::endl;
    std::cout << std::format("Errores detectados:       {}", reportar_error.size())<<std::endl;
}

//Se le ponen argumentos al main para poder capturar la informacion desde la consola
int main(int argc, char* argv[]){

    //Validacion de ingresado ruta de archivo
    if(argc <2){
        std::cerr << "No se ingreso un ruta de archivo"<<std::endl;
        return 1;
    }else if(argc > 2){
        std::cerr << "Ingreso argumentos adicionales"<<std::endl;
        return 1;
    }

    readBinaryFile(argv[1]);

    return 0;
}