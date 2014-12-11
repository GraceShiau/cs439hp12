extern "C" {
#include "libc.h"
#include "sys.h"
}

#include "libcc.h"
#include "md5.h"

/* 
 * Hashing of password happens here, reading of the passwords file happens in the kernel.
 * MD5 source: http://www.zedwood.com/article/cpp-md5-function
 * Modification of source: took out the overloading shortcut option (which takes std::string)
 * exactly 10 characters for each username
 * exactly 32 characters for hashed password
 */
 
int main() {
	while(1) {
		puts("Username: ");
		const char* username = gets();
		puts("Password: ");
		const char* password = gets();
		long len = strlen(password);

	    // hash the input password
	    MD5 md5 = MD5();
 	    md5.update(password, len);
	    md5.finalize();
	    const char* hashed = md5.hexdigest();

	    long loggedIn = CheckLoginCred(username, hashed);
		if (loggedIn > -1) {
			puts("Logged in as ");
			puts(username);
			puts(" (uid: ");
			putdec(loggedIn);
			puts(")\n");
			char** args = new char*[1];
			args[0] = "shell";
			const long err = execv(args[0], args);
			if (err == -1006) puts("Shell execution permission denied, system shutting down.");
			else if (err < 0) puts("Shell execution failed, system shutting down.");
			shutdown();
		} else puts("Authentication failed\n");
	}
}