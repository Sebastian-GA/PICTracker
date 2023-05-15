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
    return str - '0';
}

float string_to_float(char *str)
{
    float result = 0.0;
    float sign = 1.0;
    float decimal = 0.1;
    unsigned short i = 0;

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

void float_to_string(float value, char *str)
{
    // Obtener el valor absoluto del n�mero
    float abs_value = value;
    float decimal_part;
    int int_part;
    unsigned short i = 0, j = 0;

    if (abs_value < 0.0)
    {
        abs_value = -abs_value;
        str[i++] = '-';
    }

    // Convertir la parte entera a cadena
    int_part = (int)abs_value;
    if (int_part >= 100)
        str[i++] = int_part / 100 + '0'; // store hundreds
    if (int_part >= 10)
        str[i++] = (int_part / 10) % 10 + '0'; // store tens
    str[i++] = int_part % 10 + '0';            // store ones

    // Agregar el punto decimal
    str[i++] = '.';

    // Convertir la parte decimal a cadena con la precisi�n especificada
    decimal_part = abs_value - (float)int_part;
    while (j < 6) // Precisi�n de 6 decimales
    {
        decimal_part *= 10;
        int_part = (int)decimal_part;
        str[i++] = int_part + '0';
        decimal_part -= int_part;
        j++;
    }
    str[i] = '\0';
}

float sin_taylor(float x)
{
    float resultado = 0.0;
    float termino = x;
    float numerador = x;
    float denominador = 1.0;
    unsigned short n = 0;
    const unsigned short n_max = 80;

    for (n = 0; n < n_max; n++)
    {
        resultado += termino;

        numerador *= -x * x;
        denominador *= (2 * n + 2) * (2 * n + 3);
        termino = numerador / denominador;
    }

    return resultado;
}

float asin_taylor(float x)
{
    float resultado = x;
    float termino = x;
    unsigned short n = 0;
    const unsigned short n_max = 80;

    if (x > 1.0 || x < 1.0){
        PORTB.F0 = 1;
        PORTB.F0 = 0;
        return 0; // Si el valor de x est� fuera del rango v�lido [-1, 1]
    }

    for (n = 1; n < n_max; n++)
    {
        termino *= (x * x * (2 * n - 1) * (2 * n - 1)) / ((2 * n) * (2 * n + 1));
        resultado += termino;
    }

    return resultado;
}

// -------------------- FUNCIONES GPS -------------------- //
short checkGPGGA(char *text)
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
    if (text[k] == 'W')
    {
        lon_deg = -lon_deg;
    }                                // Informaci�n de direcci�n E o W
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
            r_flag = 1;
            pos = 0;
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
                    // Hacer calculos de distancia
                    distance = sin_taylor((lat_rad - lat_horse) / 2);
                    distance *= distance;
                    distance += sin_taylor(lat_horse + 1.570796) * sin_taylor(lat_rad + 1.570796) * sin_taylor((lon_rad - lon_horse) / 2) * sin_taylor((lon_rad - lon_horse) / 2);
                    distance = sqrt(distance);
                    distance = 2 * 6378.137 * asin_taylor(distance);

                    float_to_string(lat_deg, txt);
                    UART1_Write_Text(txt);
                    UART1_Write(13);
                    float_to_string(lon_deg, txt);
                    UART1_Write_Text(txt);
                    UART1_Write(13);
                    float_to_string(distance, txt);
                    UART1_Write_Text(txt);
                    UART1_Write(13);
                }
            }
            r_flag = 0; // Se apaga la bandera de llegada.
        }
    }
}
