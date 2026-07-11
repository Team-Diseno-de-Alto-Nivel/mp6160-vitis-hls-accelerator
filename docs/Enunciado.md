**Evaluación Corta 2**

**Nombre:**

**Semana # 7: 25/Junio/2026**

## **Diseño de Alto Nivel - MP6160: II Cuatrimestre 2026**

**Profesor: Luis G. León-Vega, Ph.D**

## **Instrucciones:**

Para esta evaluación, cada grupo deberá desarrollar un modelo de sistema a nivel electrónico utilizando **HLS** y **GEM5**. El objetivo es convertir el modelo de nivel de transacciones de la evaluación anterior (acelerador de procesamiento de imagen) en una implementación de hardware usando HLS. Asimismo, comunicar el modelo de nivel de transacciones con un SoC usando GEM5.

Recordando que el sistema implementa el siguiente flujo:

1. El CPU debe cargar una imagen desde un almacenamiento persistente (módulo de SC), representado por una carpeta en el computador.

2. La imagen debe estar en formato RAW RGB y corresponder a una resolución de 1080p.

3. El CPU debe almacenar la imagen en una memoria RAM de 64 MB.

4. El CPU debe indicar al acelerador:
   - La dirección base de la imagen de entrada en RAM.
   - La dirección base donde debe escribirse la imagen de salida.
   - La cantidad total de pixeles a procesar.

5. El acelerador debe leer la imagen RGB desde memoria, convertirla a escala de grises y escribir el resultado en otra región de la RAM.

6. Finalmente, el CPU debe leer la imagen procesada desde memoria y almacenarla nuevamente en disco.

## Requerimientos de la implementación de hardware

Se deben satisfacer:

- El sistema debe estar descrito en HLS usando Vitis 2024.1, usando una frecuencia de 250 MHz, con la AMD Kria KV260 como target.
- La comunicación entre los componentes debe implementarse mediante el protocolo AXI4, sea por Memory Mapped o AXI4 Stream, con control mediante AXI4-Lite.
- El acelerador debe tener un testbench en C para co-simulación.
- El acelerador debe implementar la conversión RGB a escala de grises.
- La implementación debe mostrar claramente un pipeline con separación entre:
  - Etapas de entrada/salida de datos.
  - Etapa de procesamiento.

## Requerimientos de la implementación del prototipo virtual

Se deben satisfacer:

- El modelo del acelerador debe ser el mismo que el empleado en la evaluación anterior (SystemC), con algunas adaptaciones adicionales.
- La comunicación entre el prototipo virtual y el acelerador debe realizarse mediante TLM 2.0.
- El sistema del prototipo virtual debe estar basado en ARM64.
- Debe hacerse un programa en C que interactúe con el periférico del acelerador.

## Entregables

Considere la entrega en un repositorio de GitHub con lo siguiente:

- Código fuente en HLS: implementación + testbench.
- Código fuente en SystemC del acelerador y sus auxiliares.
- Scripts TCL para correr todo el flujo de HLS.
- Scripts para automatizar la construcción del prototipo virtual y correr todo el sistema.
- Imagen de entrada en formato RAW RGB.
- Imagen de salida generada por el sistema.
- Diagrama de bloques de la arquitectura propuesta.
- README con contenido técnico explicando:
  - Instrucciones para satisfacer requisitos y compilación.
  - Organización del repo.
  - Organización de los módulos.
  - Diagrama de bloques.
  - Diagrama de secuencias.
  - Formato de las transacciones.
  - Mapa de memoria utilizado para los registros.
  - Resultados obtenidos.

## En caso de uso de Inteligencia Artificial

De acuerdo con el incentivo de uso de Inteligencia Artificial, si esta se utiliza para alguna evaluación, debe indicarse una declaración sobre su uso, incluyendo los prompts y la clase de utilización que se da, por ejemplo: revisión de código, consulta de conceptos, depuración, generación de diagramas o mejora de redacción. Una falla en esta declaración implicará la aplicación de la normativa de plagio.

**Fecha de Entrega:** 9 de Julio del 2026.
