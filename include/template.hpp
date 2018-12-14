#ifndef PANTOR_INJA_TEMPLATE_HPP
#define PANTOR_INJA_TEMPLATE_HPP


namespace inja {

class Template {
	Parsed::Element _parsed_template;

public:
	const Parsed::Element parsed_template() { return _parsed_template; }

	explicit Template(): _parsed_template(Parsed::Element()) { }
	explicit Template(const Parsed::Element& parsed_template): _parsed_template(parsed_template) { }
};

}

#endif // PANTOR_INJA_TEMPLATE_HPP
