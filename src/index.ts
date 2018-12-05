import { OCGEngine } from 'ygocore-interface';

export const engine = require('../build/Release/ocgcore') as OCGEngine<number>;
