// Programa que calcula o fatorial de 5
int main() {
    int n;
    int resultado;
    n = 5;
    resultado = 1;
    
    while (n > 0) {
        resultado = resultado * n;
        n = n - 1;
    }
    
    return resultado;
}