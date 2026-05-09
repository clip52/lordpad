# JavaScript Cheatsheet

## Variáveis
```javascript
const PI = 3.14;       // imutável (rebind)
let x = 1;             // mutável (escopo de bloco)
var legacy;            // evite (escopo de função, hoisted)
```

## Tipos
```javascript
typeof "x"            // "string"
typeof 1              // "number"
typeof 1n             // "bigint"
typeof true           // "boolean"
typeof undefined      // "undefined"
typeof null           // "object" (bug histórico)
typeof {}             // "object"
typeof []             // "object"
typeof function(){}   // "function"
Array.isArray([])     // true
```

## Strings
```javascript
const s = `Olá, ${name}!`;
s.length
s.includes("foo")
s.startsWith("a"); s.endsWith("z")
s.padStart(5, "0")
s.replace(/\s+/g, "_")
s.split(",")
s.trim(); s.trimStart(); s.trimEnd()
"a".repeat(3)         // "aaa"
```

## Arrays
```javascript
const arr = [1, 2, 3];
arr.push(4); arr.pop()
arr.unshift(0); arr.shift()
arr.slice(1, 3)
arr.splice(1, 1, "x")        // remove + insere
arr.map(x => x*2)
arr.filter(x => x > 1)
arr.reduce((acc, x) => acc + x, 0)
arr.find(x => x > 1)
arr.findIndex(x => x > 1)
arr.some(x => x > 5)
arr.every(x => x > 0)
arr.flat(2)
arr.flatMap(x => [x, x*2])
arr.includes(2)
[...arr, 4]                  // spread
[a, b, ...rest] = arr        // destructuring
Array.from({length: 3}, (_, i) => i)
```

## Objects
```javascript
const obj = { name: "alice", age: 30 };
const { name, age = 0 } = obj;
const { name: n } = obj;             // rename
const merged = { ...obj, age: 31 };  // shallow merge
Object.keys(obj)
Object.values(obj)
Object.entries(obj)
Object.fromEntries([["a",1]])
"name" in obj
delete obj.name
```

## Funções
```javascript
function fn(a, b = 0) { return a + b; }
const fn = (a, b) => a + b;          // arrow
const fn = (a, ...rest) => rest.length;
fn.bind(this, 1)
fn.call(thisArg, a, b)
fn.apply(thisArg, [a, b])

// IIFE
(() => { console.log("once"); })();
```

## Async / Promises
```javascript
async function fetchUser(id) {
    try {
        const r = await fetch(`/api/users/${id}`);
        if (!r.ok) throw new Error(`${r.status}`);
        return await r.json();
    } catch (e) {
        console.error(e);
        throw e;
    }
}

Promise.all([p1, p2]).then(([a, b]) => ...)
Promise.allSettled([p1, p2])
Promise.race([p1, p2])
Promise.any([p1, p2])

// retry helper
const retry = async (fn, n = 3) => {
    try { return await fn(); }
    catch (e) { if (n <= 1) throw e; return retry(fn, n - 1); }
};
```

## Classes
```javascript
class Point {
    #secret = 42;          // privado
    static count = 0;

    constructor(x, y) {
        this.x = x; this.y = y;
        Point.count++;
    }
    get norm() { return Math.hypot(this.x, this.y); }
    distance(other) { return Math.hypot(this.x - other.x, this.y - other.y); }

    static origin() { return new Point(0, 0); }
}

class P3 extends Point {
    constructor(x, y, z) { super(x, y); this.z = z; }
}
```

## Modules
```javascript
// math.js
export const PI = 3.14;
export function add(a, b) { return a + b; }
export default class Vec { ... }

// app.js
import Vec, { PI, add } from "./math.js";
import * as math from "./math.js";
// dynamic
const m = await import("./math.js");
```

## DOM
```javascript
document.querySelector(".foo")
document.querySelectorAll("li")
const el = document.createElement("div");
el.classList.add("active"); el.classList.toggle("on");
el.dataset.id = "42";        // → data-id="42"
el.textContent = "hi";       // safe (sem HTML)
el.innerHTML = "<b>hi</b>";  // perigoso (XSS)
parent.append(el);
el.remove();
el.addEventListener("click", e => { e.preventDefault(); ... });
el.dispatchEvent(new CustomEvent("foo", { detail: {x: 1} }));
```

## Fetch
```javascript
const r = await fetch("/api", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ x: 1 }),
    credentials: "include",
    signal: AbortSignal.timeout(5000),
});
```

## Storage
```javascript
localStorage.setItem("k", JSON.stringify(v));
const v = JSON.parse(localStorage.getItem("k") || "null");
sessionStorage  // mesmo API, escopo de sessão
```

## Optional chaining / nullish
```javascript
obj?.deep?.value ?? "default"
arr?.[0]
fn?.()
x ??= "default"        // assign if nullish
```

## Set / Map
```javascript
const s = new Set([1, 2, 2]);   // {1, 2}
s.add(3); s.has(1); s.delete(2);
s.size

const m = new Map([["a", 1]]);
m.set("b", 2); m.get("a"); m.has("a"); m.delete("a");
```
