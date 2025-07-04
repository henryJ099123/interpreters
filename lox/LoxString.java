package lox;

public class LoxString implements LoxSequence {
	private String str;

	LoxString(String str) {
		this.str = str;
	} 

	String string() {
		return this.str;
	} 

	@Override
	public int length() {return str.length();}

	@Override
	public Object getItem(int index) {
		return new LoxString(Character.toString(this.str.charAt(index)));
	} 

	@Override
	public void setItem(int index, Object value) {
		return;
	} 

	@Override
	public String toString() {
		return this.str;
	} 

	@Override
	public boolean equals(Object other) {
		if(!(other instanceof LoxString)) return false;
		return this.str.equals(((LoxString) other).string());
	} 
} 
