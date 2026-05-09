# HTML Cheatsheet

## Boilerplate
```html
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="...">
    <title>Título</title>
    <link rel="stylesheet" href="style.css">
    <link rel="icon" href="favicon.ico">
</head>
<body>
    <!-- conteúdo -->
    <script src="app.js" defer></script>
</body>
</html>
```

## Estrutura semântica
```html
<header>...</header>
<nav>...</nav>
<main>
    <article>
        <section>...</section>
    </article>
    <aside>...</aside>
</main>
<footer>...</footer>
```

## Texto
```html
<h1>...</h1> ... <h6>...</h6>
<p>parágrafo</p>
<strong>importante</strong>      <!-- semântico forte -->
<em>ênfase</em>
<mark>destaque</mark>
<small>pequeno / nota</small>
<code>inline code</code>
<pre><code>bloco code</code></pre>
<blockquote cite="url">citação</blockquote>
<abbr title="HyperText...">HTML</abbr>
<time datetime="2026-05-07">7 de maio</time>
```

## Listas
```html
<ul>
    <li>item</li>
</ul>
<ol start="3">
    <li>terceiro</li>
</ol>
<dl>
    <dt>termo</dt>
    <dd>definição</dd>
</dl>
```

## Links / mídia
```html
<a href="url">texto</a>
<a href="#section">interno</a>
<a href="mailto:x@y">email</a>
<a href="tel:+551199">telefone</a>
<a href="..." target="_blank" rel="noopener noreferrer">externo seguro</a>

<img src="x.jpg" alt="descrição" width="200" height="120" loading="lazy">
<picture>
    <source srcset="x.webp" type="image/webp">
    <img src="x.jpg" alt="...">
</picture>

<video controls poster="thumb.jpg" preload="metadata">
    <source src="v.mp4" type="video/mp4">
    <track src="legenda.vtt" kind="subtitles" srclang="pt" label="Português">
</video>

<audio controls src="a.mp3"></audio>
<iframe src="..." title="..." loading="lazy"></iframe>
```

## Forms
```html
<form action="/submit" method="post" enctype="multipart/form-data">
    <label for="email">Email:</label>
    <input type="email" id="email" name="email" required
           autocomplete="email" placeholder="alice@x.com">

    <label>
        <input type="checkbox" name="agree" required> Aceito
    </label>

    <fieldset>
        <legend>Tipo</legend>
        <label><input type="radio" name="t" value="a"> A</label>
        <label><input type="radio" name="t" value="b"> B</label>
    </fieldset>

    <select name="lang" required>
        <option value="">--</option>
        <option value="pt">Português</option>
        <option value="en">English</option>
    </select>

    <textarea name="msg" rows="4" maxlength="500"></textarea>

    <input type="file" name="upload" accept="image/*" multiple>
    <input type="date" name="dt" min="2026-01-01">
    <input type="number" name="n" min="0" max="100" step="5">
    <input type="range" name="r" min="0" max="100">
    <input type="color" name="c">
    <input type="hidden" name="csrf" value="...">

    <datalist id="cities">
        <option value="São Paulo"></option>
        <option value="Rio de Janeiro"></option>
    </datalist>
    <input list="cities" name="city">

    <button type="submit">Enviar</button>
    <button type="reset">Limpar</button>
</form>
```

## Tabelas
```html
<table>
    <caption>Vendas 2026</caption>
    <thead>
        <tr><th scope="col">Mês</th><th scope="col">Total</th></tr>
    </thead>
    <tbody>
        <tr><td>Jan</td><td>100</td></tr>
    </tbody>
    <tfoot>
        <tr><th scope="row">Total</th><td>100</td></tr>
    </tfoot>
</table>
```

## Acessibilidade
```html
<button aria-label="Fechar">×</button>
<div role="alert" aria-live="polite">...</div>
<input aria-describedby="hint">
<span id="hint">Use 8+ caracteres</span>

<!-- skip nav -->
<a class="skip" href="#main">Pular para o conteúdo</a>
```

## Atributos data-*
```html
<div data-id="42" data-state="active">...</div>
<!-- JS: el.dataset.id -->
```

## SEO/meta
```html
<meta name="robots" content="index, follow">
<meta property="og:title" content="...">
<meta property="og:image" content="...">
<meta name="twitter:card" content="summary_large_image">
<link rel="canonical" href="https://...">
```
