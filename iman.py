from PIL import Image

img_path = "switch.png"  # reemplaza con tu imagen
WIDTH, HEIGHT = 60, 22

def generar_arreglo(imagen, nombre):
    img = Image.open(imagen).convert("RGB").resize((WIDTH, HEIGHT), Image.LANCZOS)
    pixels = img.load()
    arreglo = []
    for y in range(HEIGHT):
        fila = []
        for x in range(WIDTH):
            r, g, b = pixels[x, y]
            fila.append(f"{{{r},{g},{b}}}")
        arreglo.append("  { " + ", ".join(fila) + " }")
    return f"pixelRGB {nombre}[{HEIGHT}][{WIDTH}] = {{\n" + ",\n".join(arreglo) + "\n};\n\n"

header_path = "switch.h"
with open(header_path, "w", encoding="utf-8") as f:
    f.write("struct pixelRGB { int R; int G; int B; };\n\n")
    f.write(generar_arreglo(img_path, "pc"))

print("Archivo generado:", header_path)