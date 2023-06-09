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
float temp = 0;

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
        // Verificar si el caracter es un d�gito o un punto decimal
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
                else // Caracter inv�lido, salir de la funci�n con el valor actual
                {
                    return result * sign;
                }
                i++;
            }
            break;
        }
        else // Caracter inv�lido, salir de la funci�n con el valor actual
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
    while (j < 8) // Precisi�n de 8 decimales
    {
        decimal_part *= 10;
        int_part = (int)decimal_part;
        str[i++] = int_part + '0';
        decimal_part -= int_part;
        j++;
    }
    str[i] = '\0';
}

float arcsin_newton(float x)
{
    float approximation = 0.0;
    float diff = 0.0;
    float derivative = 0.0;
    // unsigned int n = 0;

    /* if (x < -1.0 || x > 1.0)  // Nunca pasa en esta aplicaci�n
    {
        return 0.0;
    } */

    while (1) // n <= 500)
    {
        diff = sin(approximation) - x;

        if (fabs(diff) < 0.00000001)
        {
            return approximation;
        }

        derivative = cos(approximation);
        approximation = approximation - diff / derivative;

        // n++;
    }
}

float sqrt_newton(float x)
{

    float x0 = x;                   // Aproximaci�n inicial
    float x1 = (x0 + x / x0) / 2.0; // Primera iteraci�n
    // unsigned int n = 0;

    /* if (x < 0.0)  // Nunca pasa en esta aplicaci�n
    {
        return 0; // Si el n�mero es negativo, devuelve NaN (Not a Number)
    } */

    while ((fabs(x1 - x0) > 0.00000001)) // && (n <= 500))
    {
        x0 = x1;
        x1 = (x0 + x / x0) / 2.0;
        // n++;
    }

    return x1;
}

// -------------------- FUNCIONES GPS -------------------- //

short readGPS(char *text) // Convierte la trama de datos en par�metros de GPS
{
    char data_buffer[20];
    float data_minutes;
    unsigned short j, k;
    if (text[0] == '$' && text[1] == 'G' && text[2] == 'P' && text[3] == 'G' && text[4] == 'G' && text[5] == 'A')
    {
        k = 7; // Indice donde empieza informaci�n de la hora
        // Buscar el indice donde empieza la informaci�n de latitud
        while (text[k] != ',')
            k++;
        k++;                // Indice despues de la ','
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
    return 0;
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
            r_buffer[pos++] = r_char; // Se va formando la cadena de caracteres
            r_buffer[pos] = 0;        // Se limpia el siguiente byte para establecer el fin de la cadena
            break;
        }
    }
}

void send_command(char comando[15], char var[20])
{
    UART1_Write_Text(comando);
    UART1_Write_Text(var);
    UART1_Write_Text("\"");
    UART1_Write_Text("\xFF\xFF\xFF");
}
void main(void)
{
    char txt[15];
    OSCCON = 0xF0;
    TRISB = 0;
    ANSELB = 0;
    TRISB.F1 = 1;

    PIE1.RCIE = 1;
    INTCON.PEIE = 1;
    INTCON.GIE = 1;
    UART1_Init(9600);

    // -------------------- LOOP -------------------- //
    while (1)
    {
        if (r_flag) // Se ha recibido una trama
        {
            if (readGPS(r_buffer)) // Se leen los datos de las coordenadas
            {
                // Hacer calculos de distancia
                distance = sin((lat_rad - lat_horse) / 2);
                distance *= distance;
                temp = sin((lon_rad - lon_horse) / 2);
                temp *= temp;
                distance += cos(lat_horse) * cos(lat_rad) * temp;
                distance = 12756.274 * arcsin_newton(sqrt_newton(distance));

                //--------------------------Enviar datos a la pantalla NEXTION--------------------------

                float_to_string(lat_deg, txt);
                send_command("coordenada.txt=\"", txt);

                float_to_string(lon_deg, txt);
                send_command("coordenada1.txt=\"", txt);

                float_to_string(distance, txt);
                send_command("distcab.txt=\"", txt);

                UART1_Write_Text("alerta.pic=");

                if (distance > 5) // Verificar distancia
                    UART1_Write_Text("2\xFF\xFF\xFF");
                else
                    UART1_Write_Text("1\xFF\xFF\xFF");
            }
            r_flag = 0; // Se apaga la bandera de llegada.
        }
    }
}
