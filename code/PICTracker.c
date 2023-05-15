/*
    Sistema de rastreo de posici�n usando un PIC16F1827 y una pantalla Nextion.
    Si el dispositivo se encuentra por fuera del per�metro permitido se genera
    una alarma en la pantalla.

    Autores: Sebastian Garcia Angarita
             Sergio Sebastian Oliveros Sepulveda
*/

// Coordenadas del caballo de Sim�n Bolivar
const float lat_horse = 7.137734 * 0.017453292;
const float lon_horse = -73.120376 * 0.017453292;

// Variables de cordenada
float lat_deg = 0;
float lat_rad = 0;
float lon_deg = 0;
float lon_rad = 0;

// Variables para lectura de datos del gps
char r_char;
char r_buffer[100];
unsigned short pos = 0;
unsigned short r_flag = 0;

// Variables para c�lculo de distancia
float distance = 0;

// -------------------- FUNCIONES CONVERSI�N -------------------- //
int char_to_int(char str)
{
    return str - '0'; // Ver tabla ASCII
}

float string_to_float(char *str)
{
    float result = 0.0;
    float sign = 1.0;
    float decimal = 0.1;
    int i = 0;

    if (str[0] == '-') // Verifica el signo
    {
        sign = -1.0;
        i++;
    }

    while (str[i] != '\0') // Recorre los caracteres de la cadena
    {
        // Verificar si el car�cter es un d�gito o un punto decimal
        if (str[i] >= '0' && str[i] <= '9')
        {
            result = result * 10.0 + (float)char_to_int(str[i]);
        }
        else if (str[i] == '.')
        {
            i++;
            while (str[i] != '\0') // Procesar los d�gitos despu�s del punto decimal
            {
                if (str[i] >= '0' && str[i] <= '9')
                {
                    result += (float)char_to_int(str[i]) * decimal;
                    decimal *= 0.1;
                }
                else // Car�cter inv�lido, salir de la funci�n con el valor actual
                {
                    return result * sign;
                }
                i++;
            }
            break;
        }
        else // Car�cter inv�lido, salir de la funci�n con el valor actual
        {
            return result * sign;
        }
        i++;
    }

    return result * sign;
}

// -------------------- FUNCIONES GPS -------------------- //
short checkGPGGA(char *text) // Detecta si la trama inicia con los caract�res "$GPGGA"
{
    if (text[0] == '$' && text[1] == 'G' && text[2] == 'P' && text[3] == 'G' && text[4] == 'G' && text[5] == 'A')
        return 1;
    return 0;
}

short readGPS(char *text) // Convierte la trama de datos en par�metros de GPS
{
    char data_buffer[20];
    float data_minutes;
    unsigned short j, k;

    k = 7; // Indice donde empieza informaci�n de la hora
    // Buscar el indice donde empieza la informaci�n de latitud
    while (text[k] != ',')
        k++;
    k++;                // Indice despu�s de la ','
    if (text[k] == ',') // ERROR: No hay informaci�n de GPS
        return 0;

    // Informaci�n de Latitud
    j = 0;
    while (text[k] != ',')
    {
        data_buffer[j++] = text[k];
        data_buffer[j] = 0;
        k++;
    }
    // Convierte la informaci�n de grados y minutos a grados
    data_minutes = string_to_float(data_buffer + 2);
    data_buffer[2] = 0;
    lat_deg = string_to_float(data_buffer) + data_minutes / 60.0;
    k++;
    if (text[k] == 'S') // Informaci�n de direcci�n N o S
        lat_deg = -lat_deg;
    lat_rad = lat_deg * 0.017453292; // Convierte a radianes
    k++;
    k++;

    // Informaci�n de Longitud
    j = 0;
    while (text[k] != ',')
    {
        data_buffer[j++] = text[k];
        data_buffer[j] = 0;
        k++;
    }
    data_minutes = string_to_float(data_buffer + 3);
    data_buffer[3] = 0;
    lon_deg = string_to_float(data_buffer) * 1.0 + data_minutes / 60;
    k++;
    k++;
    if (text[k] == 'W') // Informaci�n de direcci�n E o W
        lon_deg = -lon_deg;
    lon_rad = lon_deg * 0.017453292; // Convierte a radianes
    // k++;
    // k++;

    return 1; // No hubo errores en la lectura
}

// -------------------- INTERRUPCIONES -------------------- //
void interrupt(void)
{
    if (PIR1.RCIF) // Se recibe un caracter por UART
    {
        r_char = UART1_Read();
        switch (r_char)
        {
        case 13: // Salto de linea
            pos = 0;
            r_flag = 1;
            break;
        case 10: // Retroceso
            break;
        default:
            r_buffer[pos++] = r_char; // Se va formando la cadena de caract�res
            r_buffer[pos] = 0;        // Se limpia el siguiente byte para establecer el fin de la cadena
            break;
        }
    }
}

void main(void)
{
    char txt[15];
    OSCCON = 0xF0;
    TRISA = 0;
    TRISB = 0;
    ANSELA = 0;
    ANSELB = 0;
    TRISB.F1 = 1;

    PIE1.RCIE = 1;
    INTCON.PEIE = 1;
    INTCON.GIE = 1;
    UART1_Init(9600);

    // -------------------- LOOP -------------------- //
    while (1)
    {
        if (r_flag) // Se ha recibido una tramas
        {
            if (checkGPGGA(r_buffer)) // Si la trama empieza con "$GPGGA"
            {
                if (readGPS(r_buffer)) // Se leen los datos de las coordenadas
                {
                    // TODO: Hacer calculos de distancia
                    distance = (1 - cos(lat_rad - lat_horse)) / 2;
                    distance *= distance;
                    distance += cos(lat_horse) * cos(lat_rad) * (1 - cos(lon_rad - lon_horse)) / 2 * (1 - cos(lon_rad - lon_horse)) / 2;
                    // distance = sqrt(distance);
                    // distance = 2 * 6378.137 * asin(distance);

                    FloatToStr(lat_deg, txt);
                    UART1_Write_Text(txt);
                }
            }
            r_flag = 0; // Se apaga la bandera de llegada.
        }
    }
}
