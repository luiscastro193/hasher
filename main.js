"use strict";
import hash from './hasher.js';

const input = document.querySelector("input");
const result = document.querySelector("p");

input.onchange = async () => {
	result.textContent = '';
	result.textContent = await hash(input.files[0].stream());
}

if (input.value) input.onchange();
