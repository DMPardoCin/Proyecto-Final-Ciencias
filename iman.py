from PIL import Image

img_path = "images.png"
WIDTH, HEIGHT = 40, 40

def generar_arreglo(imagen, nombre):
    img = Image.open(imagen).convert("RGBA").resize((WIDTH, HEIGHT), Image.LANCZOS)
    pixels = img.load()
    arreglo = []
    for y in range(HEIGHT):
        fila = []
        for x in range(WIDTH):
            r, g, b, a = pixels[x, y]
            if a < 10:
                r, g, b = 1, 1, 1
            fila.append(f"{{{r},{g},{b}}}")
        arreglo.append("  { " + ", ".join(fila) + " }")
    return f"pixelRGB {nombre}[{HEIGHT}][{WIDTH}] = {{\n" + ",\n".join(arreglo) + "\n};\n\n"

header_path = "pc.h"
with open(header_path, "w", encoding="utf-8") as f:
    f.write("struct pixelRGB { int R; int G; int B; };\n\n")
    f.write(generar_arreglo(img_path, "laser"))

print("Archivo generado:", header_path)