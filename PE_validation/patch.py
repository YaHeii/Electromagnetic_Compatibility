import nbformat

path = '/root/PE_validation/experiment1.ipynb'
with open(path, 'r', encoding='utf-8') as f:
    nb = nbformat.read(f, as_version=4)

for cell in nb.cells:
    if cell.cell_type == 'code':
        cell.source = cell.source.replace("colors = {5: 'g', 10: 'b', 15: 'r'}", "colors = {1: 'm', 5: 'g', 10: 'b', 15: 'r'}")
        cell.source = cell.source.replace("colors = {1: 'g',5: 'g', 10: 'b', 15: 'r'}", "colors = {1: 'm', 5: 'g', 10: 'b', 15: 'r'}")
        cell.source = cell.source.replace("c=colors[ws]", "c=colors.get(ws, 'k')")
        cell.source = cell.source.replace("c = colors[ws]", "c = colors.get(ws, 'k')")

with open(path, 'w', encoding='utf-8') as f:
    nbformat.write(nb, f)
print("Patch applied.")
