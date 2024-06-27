#include "auth.hpp"

Auth::Auth(const std::string& secret_key) : secret_key_(secret_key) {
}

std::string Auth::generate_token(const std::string& username) {
    auto token = jwt::create()
                            .set_subject(username)
                            .sign(jwt::algorithm::hs256{secret_key_});
    return token;
}

bool Auth::verify_token(const std::string& token, const std::string& username) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                                    .allow_algorithm(jwt::algorithm::hs256{secret_key_});

        verifier.verify(decoded);
        if (username != decoded.get_subject()){
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}
