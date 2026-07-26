# Takes a yaml file and generates a C++ binding for angelscript

def generate_comment(comment: str) -> str:
    return f"/* {comment} */"