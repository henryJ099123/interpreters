package lox;

interface LoxSequence {
	int length();
	Object getItem(int index);
	void setItem(int index, Object value);
} 
