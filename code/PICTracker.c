/*
    Sistema de rastreo de posición usando un PIC12F1822 y una pantalla Nextion.
    Si el dispositivo se encuentra por fuera del perímetro permitido se genera
    una alarma en la pantalla.

    Autores: Sebastian Garcia Angarita
             Sergio Sebastian Oliveros Sepulveda
*/

// TODO: Coordenadas del caballo de Simón Bolivar
// const float lat_horse;
// const float lon_horse;

// Variables de cordenada
char lat_dir = 0;
short lat_deg = 0;
float lat_min = 0;
char lon_dir = 0;
short lon_deg = 0;
float lon_min = 0;

// Variables para lectura de datos del gps
char r_char;
char r_bufer[100];
unsigned short pos = 0;
unsigned short r_flag = 0;

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
    char data_buffer[50];
    unsigned short j, k;

    k = 7; // Indice donde empieza información de la hora
    // Buscar el indice donde empieza la información de latitud
    while (text[k] != ',')
        k++;
    k++;                // Indice después de la ','
    if (text[k] == ',') // ERROR: No hay información de GPS
        return 0;

    // Convertir información de Latitud
    j = 0;
    while (text[k] != ',')
    {
        data_buffer[j++] = text[k];
        data_buffer[j] = 0;
        k++;
    }
    // TODO: data_buffer --> Información de Latitud
    // data_buffer[0:2] & data_buffer[2:end]
    k++;
    lat_dir = text[k]; // Información de dirección N o S
    k++;
    k++;

    // Convertir información de Longitud
    j = 0;
    while (text[k] != ',')
    {
        data_buffer[j++] = text[k];
        data_buffer[j] = 0;
        k++;
    }
    // TODO: data_buffer --> Información de Longitud
    // data_buffer[0:3] & data_buffer[3:end]
    k++;
    lon_dir = text[k]; // Información de dirección W o E
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
            r_bufer[pos++] = r_char; // Se va formando la cadena de caractéres
            r_bufer[pos] = 0;        // Se limpia el siguiente byte para establecer el fin de la cadena
            break;
        }
    }
}

void main(void)
{
    OSCCON = 0xF0;
    TRISA = 0;
    ANSELA = 0;

    PIE1.RCIE = 1;
    INTCON.PEIE = 1;
    INTCON.GIE = 1;
    UART1_Init(4800);

    // -------------------- LOOP -------------------- //
    while (1)
    {
        if (r_flag) // Se ha recibido una tramas
        {
            if (checkGPGGA(r_bufer)) // Si la trama empieza con "$GPGGA"
            {
                if (readGPS(r_bufer)) // Se leen los datos de las coordenadas
                {
                    // TODO: Hacer calculos de distancia
                }
            }
            r_flag = 0; // Se apaga la bandera de llegada.
        }
    }
}
