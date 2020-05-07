# Reto: Desarrollar un driver
- Curso: Sistemas Operativos
- Estudiante: Sebas Arriola

> Nota: el código fue probado únicamente en ubuntu 18.04

## Instrucciones para probar chardev
1. Entrar al directorio chardev.
2. Ejecutar `sudo make`
3. Se creará el archivo chardev.ko
4. Ejecutar  `sudo insmod chardev.ko` para insertar el módulo al kernel.
5. Ejecutar `dmesg` para asegurarse que el módulo fue cargado correctamente, y anotar el número MAJOR.
6. Crear un device file para comunicarse con nuestro driver con el comando `mknod /dev/chardev0 c MAJOR 0`. Sustituir MAJOR con el número anotado anteriormente.
7. Se puede hacer un programa en C (ver archivo test_chardev.c) para tener un ejemplo de como comunicarse con el driver.

## Instrucciones para probar driver de control de ps3 sixaxis
1. Entrar al directorio ps3_sixaxis
2. Ejecutar `sudo make`
3. Se creará el archivo ps3_sixaxis.ko
4. Ejecutar  `sudo insmod ps3_sixaxis.ko` para insertar el módulo al kernel.
5. Ejecutar `dmesg` para asegurarse que el módulo fue cargado correctamente.
6. Conectar el dispositivo sixaxis a la computadora.
7. Revisar el output de `dmesg` para comprobar que el driver "sixaxis_driver" aparece en el output.
8. Presionar el boton PS del control sixaxis.
9. Mover el joystick izquierdo y comprobar que las luces LEDs del control se mueven, siguiendo al joystick.

> Video de funcionamiento: https://youtu.be/RRP88bjE5_4


