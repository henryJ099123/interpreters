package lox;

import java.util.List;
import java.util.ArrayList;

public class LoxList implements LoxSequence {
	final List<Object> list = new ArrayList<>();

	LoxList(int size) {
		for(int i = 0; i < size; i++)
			list.add(null);
	}

	LoxList(List<Object> items) {
		for(Object item: items)
			list.add(item);
	}

	public void append(Object value) {
		list.add(value);
	} 

	@Override
	public void setItem(int index, Object value) {
		list.set(index, value);
	} 

	@Override
	public Object getItem(int index) {
		return list.get(index);
	} 

	@Override
	public int length() {
		return list.size();
	} 

	@Override
	public String toString() {
		String text = "[";
		for(Object item: this.list)
			text += Interpreter.stringify(item) + ", ";
		text = text.substring(0, text.length() - 2);
		return text + "]";
	} 

	@Override
	public boolean equals(Object other) {
		if(!(other instanceof LoxList)) return false;
		return this.list.equals(((LoxList) other).list);
	} 
} 
