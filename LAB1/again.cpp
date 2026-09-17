#include <fstream>
#include <format>
#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

//Creador de errores
struct error{
    size_t offset;
    std::string message;
};


//cargar archivo
void converter(const std::string& fileName){
    //apertura de archivo
    std::ifstream in(fileName, std::ios::binary | std::ios::ate);
    if(!in.is_open()){
        throw std::runtime_error("Error al abrir archivo");
    }

    //obtener el size
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    
    std::vector<uint32_t> buffer(size);

    //deserialziar
    if(!in.read(reinterpret_cast<char*>(buffer.data()), size)){
        std::cerr << "ERROR DE DESERIALIZAR";
        in.close();
        return;
    }
    in.close();

    //variables a usar
    std::vector<error> errores;
    std::vector<uint32_t> code_points;
    int bytesCounter[4] = {0,0,0,0};
    size_t offset=0;

    //Evaluacion de BOM
    if(buffer.size()> 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF ){
        offset=3;
    }

    while(offset < buffer.size()){
        uint8_t b1 = buffer[offset];
        uint32_t code_point = 0x00;

        //1 byte
        if((b1 & 0x80) == 0x00){
            code_point = b1;
            code_points.push_back(code_point);
            offset+=1;
            bytesCounter[0]++;

        }else if((b1 & 0xE0) == 0xC0){
            if(offset+1 > buffer.size()){
                error er;
                er.offset= offset;
                er.message= "Error secuencia incompleta";
                errores.push_back(er);
                offset+=1;
                continue;
            }

            uint8_t b2 = buffer[offset+1];

            if((b2 & 0xC0) != 0x80){
                error er;
                er.offset= offset;
                er.message= "Error en byte de continuacion";
                errores.push_back(er);
                offset+=1;
                continue;
            }

            code_point = ((b1 & 0x1F) << 6) | (b2 & 0x3F);

            if(code_point < 0x80){
                error er;
                er.offset= offset;
                er.message= "Error de byte sobrelargo";
                errores.push_back(er);
                offset+=1;
                continue;
            }

            code_points.push_back(code_point);
            bytesCounter[1]++;
            offset+=2;


        }else if((b1 & 0xF0) ==  0xE0){

            if(offset+2 > buffer.size()){
                error er;
                er.offset= offset;
                er.message= "Error secuencia incompleta";
                errores.push_back(er);
                offset+=1;
                continue;
            }


            uint8_t b2 = buffer[offset+1];
            uint8_t b3 = buffer[offset+2];

            if((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80){
                error er;
                er.offset= offset;
                er.message= "Error en byte de continuacion";
                errores.push_back(er);
                offset+=1;
                continue;
            }

            code_point = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);

            if(code_point < 0x800){
                error er;
                er.offset= offset;
                er.message= "Error de byte sobrelargo";
                errores.push_back(er);
                offset+=1;
                continue;
            }

            code_points.push_back(code_point);
            bytesCounter[2]++;
            offset+=3;


        }else if((b1 & 0xF8) == 0xF0){
            if(offset+3 > buffer.size()){
                error er;
                er.offset= offset;
                er.message= "Error secuencia incompleta";
                errores.push_back(er);
                offset+=1;
                continue;
            }


            uint8_t b2 = buffer[offset+1];
            uint8_t b3 = buffer[offset+2];
            uint8_t b4 = buffer[offset+3];

            if((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80 || (b4 & 0xC0) != 0x80){
                error er;
                er.offset= offset;
                er.message= "Error en byte de continuacion";
                errores.push_back(er);
                offset+=1;
                continue;
            }

            code_point = ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) <<6) | (b4 & 0x3F);

            if(code_point < 0x10000 || code_point > 0x10FFFF ){
                error er;
                er.offset= offset;
                er.message= "Error de byte sobrelargo";
                errores.push_back(er);
                offset+=1;
                continue;
            }

            code_points.push_back(code_point);
            bytesCounter[3]++;
            offset+=4;

        }else if((b1 & 0xC0) == 0x80){
            error er;
            er.offset= offset;
            er.message= "Byte de continuacion inesperado";
            errores.push_back(er);
            offset+=1;
            continue;

        }else{
            error er;
                er.offset= offset;
                er.message= "byte lider incorrecto";
                errores.push_back(er);
                offset+=1;
                continue;

        }
    }

//decodificador
for(uint32_t cp : code_points){
    if(cp >= 0x20 && cp <= 0x7E){
        std::cout << static_cast<char>(cp)<<std::endl;
    }else{
        std::cout << std::format("U+{:4X}", cp);
    }
}


}

int main(int argc, char* argv[]){
    if(argc != 2){
        std::cerr <<"Parametros faltantes"<<std::endl;
        return 1;
    }

    converter(argv[1]);

    return 0;
}