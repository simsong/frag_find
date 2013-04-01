
// Functions:
// rs = stmt.executeQuery
// stmt.execute() & smtm.getResultSet()
// rs.next()
// rs.getInt("name")
// rs.getString("name")
// API at http://java.sun.com/j2se/1.5.0/docs/api/java/sql/ResultSet.html


import java.sql.*;			

// or 
//import java.sql.Connection;
//import java.sql.DriverManager;
//import java.sql.SQLException;
//import java.sql.Statement;
//import java.sql.ResultSet;

//http://dev.mysql.com/doc/refman/5.0/en/



public class Jdbc12 {
    public static void main(String args[]){

	System.out.println("Your MySQL Password is "+System.getenv("MP"));
	String MP = System.getenv("MP");
	String username = System.getenv("USER");

	try {
	    //Register the JDBC driver for MySQL.

	    Class.forName("com.mysql.jdbc.Driver");
	    // or java -Djdbc.drivers=com.mysql.jdbc.Driver
	    // see http://www.stardeveloper.com/articles/display.html?article=2003090401&page=1

	    // Specify the URL of database server for
	    String url = "jdbc:mysql://localhost:3306/mysql";

	    // Create the database connection
	    Connection con = DriverManager.getConnection(url,username,MP);

	    //Display URL and connection information
	    System.out.println("URL: " + url);
	    System.out.println("Connection: " + con);

	    //Get a Statement object
	    Statement stmt = con.createStatement();

	    //Remove the user named auser
	    //stmt.executeUpdate("show databases");
	    stmt.executeUpdate("use s3");
	    //ResultSet rs = stmt.executeQuery("show databases");
	    ResultSet rs = stmt.executeQuery("select * from dns");
	    while(rs.next()){
		String host = rs.getString("host");
		System.out.println("host = "+host);
	    }
	    rs.close();
	    con.close();
	} catch( Exception e ) {
	    e.printStackTrace();
	}//end catch
    }//end main
}//end class Jdbc12

