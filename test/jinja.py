from jinja2 import Environment, PackageLoader, select_autoescape


if __name__ == '__main__':
    env = Environment(
        loader=PackageLoader("jinja"),
        autoescape=select_autoescape()
    )

    template = env.get_template("example.txt")
    print(template.render(name="Jeff"))
