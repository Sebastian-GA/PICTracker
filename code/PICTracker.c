/*
    Sistema de rastreo de posición usando un PIC16F1827 y una pantalla Nextion.
    Si el dispositivo se encuentra por fuera del perímetro permitido se genera
    una alarma en la pantalla.

    Autores: Sebastian Garcia Angarita
             Sergio Sebastian Oliveros Sepulveda
*/

// Coordenadas del caballo de Simón Bolivar
const float lat_horse = 7.137734;
const float lon_horse = -73.120376;

// Variables de cordenada
float lat_deg = 0;
float lon_deg = 0;

// Variables para lectura de datos del gps
char r_char;
char r_buffer[100];
unsigned short pos = 0;
unsigned short r_flag = 0;

// Variables para cálculo de distancia
float distance = 0;

// Detecta si la trama inicia con los caractéres "$GPGGA"
short checkGPGGA(char *text)
{
    if (text[0] == '$' && text[1] == 'G' && text[2] == 'P' && text[3] == 'G' && text[4] == 'G' && text[5] == 'A')
        return 1;
    return 0;
}

// Convertir la trama de datos en parámetros de GPS
short readGPS(char *text)
{
    char data_buffer[20];
    float data_minutes;
    unsigned short j, k;

    k = 7; // Indice donde empieza información de la hora
    // Buscar el indice donde empieza la información de latitud
    while (text[k] != ',')
        k++;
    k++;                // Indice después de la ','
    if (text[k] == ',') // ERROR: No hay información de GPS
        return 0;

    // Información de Latitud
    j = 0;
    while (text[k] != ',')
    {
        data_buffer[j++] = text[k];
        data_buffer[j] = 0;
        k++;
    }
    // Convierte la información de grados y minutos a grados
    data_minutes = atof(data_buffer + 2);
    data_buffer[2] = 0;
    lat_deg = atoi(data_buffer) * 1.0 + data_minutes / 60;
    k++;
    if (text[k] == 'S') // Información de dirección N o S
        lat_deg = -lat_deg;
    k++;
    k++;

    // Información de Longitud
    j = 0;
    while (text[k] != ',')
    {
        data_buffer[j++] = text[k];
        data_buffer[j] = 0;
        k++;
    }
    data_minutes = atof(data_buffer + 3);
    data_buffer[3] = 0;
    lon_deg = atoi(data_buffer) * 1.0 + data_minutes / 60;
    k++;
    k++;
    if (text[k] == 'W') // Información de dirección E o W
        lon_deg = -lon_deg;
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
            r_buffer[pos++] = r_char; // Se va formando la cadena de caractéres
            r_buffer[pos] = 0;        // Se limpia el siguiente byte para establecer el fin de la cadena
            break;
        }
    }
}

void main(void)
{
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
                }
            }
            r_flag = 0; // Se apaga la bandera de llegada.
        }
    }
}
