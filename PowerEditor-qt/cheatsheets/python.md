# Python Cheatsheet

## Tipos básicos
```python
i = 42                  # int
f = 3.14                # float
s = "hello"             # str
b = b"\x00\x01"         # bytes
t = (1, 2, 3)           # tuple (imutável)
l = [1, 2, 3]           # list
d = {"a": 1}            # dict
st = {1, 2, 3}          # set
```

## f-strings
```python
name = "alice"
f"Olá, {name}!"
f"{x:>10}"              # right-align width 10
f"{x:.2f}"              # 2 casas
f"{x:08x}"              # hex 8-wide zero-pad
f"{value=}"             # debug: "value=42"
```

## Comprehensions
```python
[x*2 for x in range(10) if x % 2]
{x: x**2 for x in range(5)}
{x.lower() for x in items}
gen = (x*2 for x in iterable)   # generator (lazy)
```

## Funções
```python
def fn(a, b=2, *args, key=None, **kw):
    """docstring."""
    return a + b

# Type hints
def add(a: int, b: int) -> int:
    return a + b

# Lambda
square = lambda x: x*x
```

## Classes
```python
class Point:
    def __init__(self, x: float, y: float):
        self.x = x; self.y = y
    def __repr__(self):
        return f"Point({self.x}, {self.y})"

from dataclasses import dataclass
@dataclass(frozen=True, slots=True)
class P:
    x: int
    y: int
```

## Context managers
```python
with open("f.txt") as f:
    data = f.read()

from contextlib import contextmanager
@contextmanager
def my_ctx():
    setup(); yield; teardown()
```

## Async
```python
import asyncio
async def fetch(url):
    await asyncio.sleep(1)
    return "ok"

asyncio.run(fetch("..."))

# gather várias tasks em paralelo
results = await asyncio.gather(fetch("a"), fetch("b"))
```

## Itertools muito usados
```python
from itertools import chain, groupby, product, combinations
list(chain(a, b))                  # concat
[(k, list(g)) for k, g in groupby(items, key=len)]
list(product([1,2], [3,4]))        # produto cartesiano
list(combinations([1,2,3], 2))     # pares únicos
```

## Pathlib
```python
from pathlib import Path
p = Path("/tmp/x.txt")
p.exists()
p.read_text()
p.write_text("hi")
p.parent / "y.txt"                  # join
p.stem; p.suffix; p.name
list(Path(".").rglob("*.py"))       # recursive glob
```

## Estruturas avançadas
```python
from collections import Counter, defaultdict, deque, OrderedDict
Counter("aabbc").most_common()      # [('a',2),('b',2),('c',1)]
dd = defaultdict(list); dd["k"].append(1)
dq = deque(maxlen=10); dq.append(x)
```

## Erros comuns
```python
try:
    risky()
except (ValueError, KeyError) as e:
    log(e)
except Exception:
    raise
else:
    success()
finally:
    cleanup()
```

## Virtualenv & pip
```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
pip freeze > requirements.txt
pip install -e ".[dev]"   # editable + extras
```

## Debug rápido
```python
breakpoint()              # built-in pdb
import pdb; pdb.set_trace()
print(f"{var=}")          # "var=42"
import traceback; traceback.print_exc()
```
