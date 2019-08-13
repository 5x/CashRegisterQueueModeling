#include "CashRegisterQueueModeling.h"


const std::string COMMAND_HELP = "help";
const std::string COMMAND_ABOUT = "about";
const std::string COMMAND_START = "start";
const std::string COMMAND_CURRENCY = "currency";
const std::string COMMAND_PAID_SETTINGS = "paid_settings";
const std::string COMMAND_CREATE_PRODUCT = "create_product";
const std::string COMMAND_CREATE_USER = "create_user";
const std::string COMMAND_VIEW_PRODUCT = "view_product";
const std::string COMMAND_VIEW_USER = "view_user";
const std::string COMMAND_VIEW_USER_CART = "view_cart";
const std::string COMMAND_CREATE_CONFIGURATION = "create_configuration";
const std::string COMMAND_VIEW_CONFIGURATION = "view_configuration";
const std::string COMMAND_EXIT = "exit";

const int DEFAULT_MIN_USERS_IN_QUEUE = 2;
const int DEFAULT_MAX_USERS_IN_QUEUE = 25;
const int DEFAULT_MAX_PROCESSING_TIME = 60;
const int DEFAULT_MAX_DIGITAL_PROCESSING_TIME = 60;
const int DEFAULT_MAX_SERVICE_PROCESSING_TIME = 180;
const int DEFAULT_MAX_REGISTRATION_PROCESSING_TIME = 365;
const int DEFAULT_EFFECTIVITY_RATE = 100;
const int DEFAULT_ADDITIONLA_PAID_TIME_IN_SEC = 10;
const int MAX_POSSIBLE_NUM_OF_CASH_REGISTERS = 320000;

const std::string PRODUCT_DB_FILE_NAME = "products.txt";
const std::string USERS_DB_FILE_NAME = "users.txt";
const std::string CONFIGURATION_DB_FILE_NAME = "settings.txt";
const std::string TEMP_FILE_NAME = "temp.txt";


enum PaymentMethods {
	MaxAvailableAmount,
	PayByAllPosibble,
	OnlyCash,
	OnlyCreditCards,
	OnlyDigitalWallet,
	TogetherCashAndCreditCards,
	UseWithoutPaymentRestrics,
};

struct User {
	std::string name;
	double amount_cash;
	double amount_on_card;
	double amount_digital;
	std::vector<int> products_cart;
};

struct Product {
	std::string name;
	double price;
	int type;
	int amount;
};

struct Configuration {
	std::string name;
	int max_users_in_queue;
	int default_processing_time;
	int digital_processing_time;
	int service_processing_time;
	int registration_processing_time;
	int effectivity_rate;
};


double currency_rate = 1;
int paid_time = DEFAULT_ADDITIONLA_PAID_TIME_IN_SEC;
PaymentMethods current_pay_method = MaxAvailableAmount;


std::vector<User> users;
std::vector<Product> products;
std::vector<Configuration> configurations;


void execute_command(const std::string& command);


std::vector<std::string> split(const std::string& s, const char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream token_stream(s);

	while (std::getline(token_stream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

int parse_int(const char* buff) {
	char* end;
	errno = 0;
	const long sl = strtol(buff, &end, 10);

	if (end == buff || '\0' != *end ||
		(LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno ||
		sl > INT_MAX || sl < INT_MIN) {
		throw std::out_of_range("Can\'t be parsed as integer.");
	}

	return (int)sl;
}

double parse_double(const char* buff) {
	double value = std::atof(buff);

	if (strlen(buff) == 0 || (value == 0.0 && buff[0] != '0')) {
		throw std::out_of_range("Can\'t be parsed as double.");
	}

	return value;
}

int parse_int(const std::string& buff) {
	return parse_int(buff.c_str());
}

double parse_double(const std::string& buff) {
	return parse_double(buff.c_str());
}

void load_from_file(std::string filename, void handle(std::vector<std::string>)) {
	const char DELIMETER = '\t';
	std::vector<std::string> tokens;

	std::string line;
	std::ifstream file(filename);

	if (file.fail()) {
		std::cout << "File[\"" << filename << "\"] not found, something wrong..." << std::endl;

		std::cin.get();
		exit(1);
	}

	while (std::getline(file, line)) {
		tokens = split(line, DELIMETER);
		handle(tokens);
	}

	file.close();
}

void print_file_to_console(const std::string& filename) {
	std::string line;
	std::ifstream file(filename);

	if (file.fail()) {
		std::cout << "Can\'t read \"" << filename << "\" file." << std::endl;
	} else {
		while (std::getline(file, line)) {
			std::cout << line << std::endl;
		}
	}

	file.close();
}

template <class T>
T get_or_default(std::vector<T> values, int index, T default_value) {
	try {
		return values.at(index);
	} catch (const std::out_of_range&) {
		return default_value;
	}
}

double get_or_default(std::vector<std::string> values, int index, double default_value) {
	try {
		std::string value = values.at(index);
		return parse_double(value);
	} catch (const std::out_of_range&) {
		return default_value;
	}
}

int get_or_random(std::vector<std::string> values, int index, int min_range, int max_range) {
	try {
		std::string value = values.at(index);
		return parse_int(value);
	} catch (const std::out_of_range&) {
		max_range = max_range - min_range + 1;
		return min_range + std::rand() % max_range;
	}
}

void create_user(
	const std::string& name,
	double amount_cash,
	double amount_on_card,
	double amount_digital,
	std::vector<int> products_cart
) {
	User user;

	if (amount_cash < 0) {
		amount_cash = 0;
	}

	if (amount_on_card < 0) {
		amount_on_card = 0;
	}

	if (amount_digital < 0) {
		amount_digital = 0;
	}

	if (name.empty()) {
		user.name = "Unnamed";
	} else {
		user.name = name;
	}

	user.name = name;
	user.amount_cash = amount_cash;
	user.amount_on_card = amount_on_card;
	user.amount_digital = amount_digital;
	user.products_cart = products_cart;

	users.push_back(user);
}

void create_product(const std::string& name, double price, int type, int amount) {
	Product product;

	if (price < 0) {
		price = 0.0;
	}

	if (type < 0) {
		type = 0;
	}

	if (amount < 0) {
		amount = 0;
	}

	if (name.empty()) {
		product.name = "Unnamed";
	} else {
		product.name = name;
	}

	product.price = price;
	product.type = type;
	product.amount = amount;

	products.push_back(product);
}

void create_configuration(
	const std::string& name,
	int max_users_in_queue,
	int default_processing_time,
	int digital_processing_time,
	int service_processing_time,
	int registration_processing_time,
	int effectivity_rate
) {
	Configuration configuration;

	if (max_users_in_queue < 0) {
		max_users_in_queue = 0;
	}

	if (default_processing_time < 0) {
		default_processing_time = 0;
	}

	if (digital_processing_time < 0) {
		digital_processing_time = 0;
	}

	if (service_processing_time < 0) {
		service_processing_time = 0;
	}

	if (registration_processing_time < 0) {
		registration_processing_time = 0;
	}

	if (effectivity_rate < 0 || effectivity_rate > 100) {
		effectivity_rate = 100;
	}

	if (name.empty()) {
		configuration.name = "Unnamed";
	} else {
		configuration.name = name;
	}

	configuration.name = name;
	configuration.max_users_in_queue = max_users_in_queue;
	configuration.default_processing_time = default_processing_time;
	configuration.digital_processing_time = digital_processing_time;
	configuration.service_processing_time = service_processing_time;
	configuration.registration_processing_time = registration_processing_time;
	configuration.effectivity_rate = effectivity_rate;

	configurations.push_back(configuration);
}

std::vector<int> get_product_ids(std::vector<std::string> products_cart) {
	int num_of_products = 1 + rand() % products_cart.size();
	std::vector<int> cart_ids;

	for (std::vector<std::string>::iterator it = products_cart.begin();
		it != products_cart.end() && num_of_products >= 0;
		++it, --num_of_products) {
		int value = parse_int(*it);

		if (value != NULL) {
			cart_ids.push_back(value);
		}
	}

	return cart_ids;
}

void user_parser(std::vector<std::string> values) {
	if (values.empty()) {
		return;
	}

	std::string name = get_or_default<std::string>(values, 0, "Unnamed");
	double amount_cash = get_or_default(values, 1, 0.0);
	double amount_on_card = get_or_default(values, 2, 0.0);
	double amount_digital = get_or_default(values, 3, 0.0);

	const char DELIMETER = ' ';
	std::string product_cart_str = get_or_default<std::string>(values, 4, "");
	std::vector<std::string> products_cart = split(product_cart_str, DELIMETER);
	std::vector<int> cart_ids = get_product_ids(products_cart);

	create_user(name, amount_cash, amount_on_card, amount_digital, cart_ids);
}

void product_parser(std::vector<std::string> values) {
	if (values.empty()) {
		return;
	}

	std::string name = get_or_default<std::string>(values, 0, "Unnamed");
	double price = get_or_default(values, 1, 0.0);
	int type = get_or_random(values, 2, 0, 0);
	int amount = get_or_random(values, 3, 0, 0);

	create_product(name, price, type, amount);
}

void configuration_parser(std::vector<std::string> values) {
	if (values.empty()) {
		return;
	}

	std::string name = get_or_default<std::string>(values, 0, "Unnamed");
	int max_users_in_queue = get_or_random(values, 1, DEFAULT_MIN_USERS_IN_QUEUE, DEFAULT_MAX_USERS_IN_QUEUE);
	int default_processing_time = get_or_random(values, 2, 0, DEFAULT_MAX_PROCESSING_TIME);
	int digital_processing_time = get_or_random(values, 3, 0, DEFAULT_MAX_DIGITAL_PROCESSING_TIME);
	int service_processing_time = get_or_random(values, 4, 0, DEFAULT_MAX_SERVICE_PROCESSING_TIME);
	int registration_processing_time = get_or_random(values, 5, 0, DEFAULT_MAX_REGISTRATION_PROCESSING_TIME);
	int effectivity_rate = get_or_random(values, 6, 0, DEFAULT_EFFECTIVITY_RATE);

	create_configuration(name, max_users_in_queue, default_processing_time,
		digital_processing_time, service_processing_time,
		registration_processing_time, effectivity_rate
	);
}

void load_data() {
	load_from_file(USERS_DB_FILE_NAME, user_parser);
	load_from_file(PRODUCT_DB_FILE_NAME, product_parser);
	load_from_file(CONFIGURATION_DB_FILE_NAME, configuration_parser);
}

bool is_reserved_command(std::string value) {
	return value == COMMAND_HELP ||
		value == COMMAND_ABOUT ||
		value == COMMAND_START ||
		value == COMMAND_CURRENCY ||
		value == COMMAND_PAID_SETTINGS ||
		value == COMMAND_CREATE_PRODUCT ||
		value == COMMAND_CREATE_USER ||
		value == COMMAND_VIEW_PRODUCT ||
		value == COMMAND_VIEW_USER ||
		value == COMMAND_VIEW_USER_CART ||
		value == COMMAND_CREATE_CONFIGURATION ||
		value == COMMAND_VIEW_CONFIGURATION ||
		value == COMMAND_EXIT;
}

std::string user_input(const std::string& message) {
	std::cout << message;

	std::string input;
	std::getline(std::cin, input);

	return input;
}

std::string user_handle(const std::string& message) {
	std::string input_string = user_input(message);

	if (is_reserved_command(input_string)) {
		execute_command(input_string);
		exit(0);
	}

	if (input_string.empty()) {
		std::cout << "Incorrect or empty value input. Try Again." << std::endl;
		return user_handle(message);
	}

	return input_string;
}

int user_handle_int(const std::string& message) {
	std::string input_string = user_handle(message);
	int value;

	try {
		value = parse_int(input_string);
	} catch (const char*) {
		std::cout << "Input value can\' be converted to number. Try again." << std::endl;

		return user_handle_int(message);
	}

	return value;
}

double user_handle_double(const std::string& message) {
	std::string input_string = user_handle(message);
	double value;

	try {
		value = parse_double(input_string);
	} catch (const char*) {
		std::cout << "Input value can\' be converted to number. Try again." << std::endl;

		return user_handle_double(message);
	}

	return value;
}

void handle_help() {
	std::cout << COMMAND_HELP << " - List of available commands with description." << std::endl
		<< COMMAND_START << " - Start modeling processing of calculation queues cash register." << std::endl
		<< COMMAND_ABOUT << " - Application about information." << std::endl
		<< COMMAND_CURRENCY << " - Define currency of input values." << std::endl
		<< COMMAND_PAID_SETTINGS << " - Change payment settings." << std::endl
		<< COMMAND_CREATE_USER << " - Register new user." << std::endl
		<< COMMAND_VIEW_USER << " - View user information." << std::endl
		<< COMMAND_VIEW_USER_CART << " - View user wish cart." << std::endl
		<< COMMAND_CREATE_PRODUCT << " - Register new product on the store." << std::endl
		<< COMMAND_VIEW_PRODUCT << " - View product details." << std::endl
		<< COMMAND_CREATE_CONFIGURATION << " - Create new configuration." << std::endl
		<< COMMAND_VIEW_CONFIGURATION << " - View queue configuration" << std::endl
		<< COMMAND_EXIT << " - Close program session." << std::endl
		<< std::endl;
}

void handle_about() {
	std::cout << "This software product was developed as part of the 1st course work." << std::endl
		<< "Subject: Programming an assessment of the impact of customer parameters on the queue size." << std::endl
		<< "Version: 1.0.2(07.04.2013)" << std::endl
		<< "Author: Serhii Zarutskiy." << std::endl
		<< std::endl;
}

std::vector<std::vector<User>> generate_queue(Configuration cfg, int num_of_cash_registers) {
	size_t num_of_users = users.size();
	std::vector<std::vector<User>> queues;

	for (int i = 0; i < num_of_cash_registers; i++) {
		int queue_size = rand() % cfg.max_users_in_queue;
		std::vector<User> queue;

		for (int j = 0; j < queue_size; j++) {
			int random_user_id = rand() % num_of_users;
			User user = users.at(random_user_id);
			queue.push_back(user);
		}

		queues.push_back(queue);
	}

	return queues;
}

std::string time_to_string(int seconds) {
	int min = seconds / 60;
	int sec = seconds % 60;

	return std::to_string(min) + ":" + (sec < 10 ? "0" : "") + std::to_string(sec);
}

void show_available_configuration_list() {
	std::cout << "Available configuration:" << std::endl;

	for (std::vector<Configuration>::iterator it = configurations.begin();
		it != configurations.end(); ++it) {
		std::cout << (*it).name << " \\ ";
	}

	std::cout << std::endl;
}

Configuration get_configuration_by_name(const std::string& configuration_name) {
	for (std::vector<Configuration>::iterator it = configurations.begin();
		it != configurations.end(); ++it) {
		if ((*it).name == configuration_name) {
			return *it;
		}
	}

	throw std::out_of_range("Invalid configuration name.");
}

void handle_start() {
	const std::string promt_message = "Enter the number of cash registers: ";

	int num_of_cash_registers;
	try {
		num_of_cash_registers = user_handle_int(promt_message);

		if (num_of_cash_registers <= 0 || num_of_cash_registers > MAX_POSSIBLE_NUM_OF_CASH_REGISTERS) {
			throw std::out_of_range("Value out of expected range.");
		}
	} catch (std::out_of_range&) {
		std::cout << "Selected number of cash registers can\'t be handle." << std::endl;
		return;
	}

	show_available_configuration_list();
	std::string promt_cfg_message = "Type name of configuration: ";
	std::string configuration_name = user_handle(promt_cfg_message);

	Configuration cfg;
	try {
		cfg = get_configuration_by_name(configuration_name);
	} catch (std::out_of_range&) {
		std::cout << "Configuration with specified name not found!" << std::endl;
		return;
	}

	std::vector<std::vector<User>> mas = generate_queue(cfg, num_of_cash_registers);

	std::ofstream fout(TEMP_FILE_NAME, std::ios_base::out | std::ios_base::trunc);
	if (!fout.is_open()) {
		std::cout << "Can\' create tempory file." << std::endl;
		return;
	}
	fout << std::fixed << std::setprecision(2);

	std::cout << "Creating tempory files..." << std::endl
		<< "Evaluation queue..." << std::endl;

	// PART OF PROGRAM HIDDEN BY AUTHOR.

	fout.close();

	print_file_to_console(TEMP_FILE_NAME);

	const std::string promt_save_message = "Save result to file[Y\\n]: ";
	std::string answer = user_handle(promt_save_message);

	if (answer == "Y") {
		const std::string promt_filename_message = "Filename: ";
		std::string new_filename = user_handle(promt_filename_message);

		const char* new_filename_chars = new_filename.c_str();
		const char* current_filename = TEMP_FILE_NAME.c_str();
		bool rename_check_code = rename(current_filename, new_filename_chars);

		if (rename_check_code == 0) {
			std::cout << "Result saved to: " << new_filename << std::endl;
		} else {
			std::cout << "Error. Can\'t save, maybe file already exist." << std::endl;
		}
	}
}

void handle_currency() {
	std::cout << "Result of all calculation will be show in initial currency(UAH)." << std::endl
		<< "Type currency designator to change current calculation coefficient." << std::endl
		<< "UAH(980) - Ukrainian hryvnia, initial value;" << std::endl
		<< "USD(840) - United States dollar;" << std::endl
		<< "EUR(978) - Euro;" << std::endl
		<< "GBP(826) - Pound sterling;" << std::endl
		<< "JPY(392) - Japanese yen." << std::endl
		<< std::endl;

	const std::string promt_message = "Type currency code: ";
	int currency_code = user_handle_int(promt_message);

	switch (currency_code) {
	case 980:
		::currency_rate = 1;
		break;
	case 840:
		::currency_rate = 8.1;
		break;
	case 978:
		::currency_rate = 10.4;
		break;
	case 826:
		::currency_rate = 12.8;
		break;
	case 392:
		::currency_rate = 0.085;
		break;
	default:
		std::cout << "Invalid currency code value." << std::endl;
		return;
	}

	std::cout << "Currency changed." << std::endl;
}

void select_payment_method_by_id(int payment_id) {
	switch (payment_id) {
	case 1:
		::current_pay_method = MaxAvailableAmount;
		break;
	case 2:
		::current_pay_method = PayByAllPosibble;
		break;
	case 3:
		::current_pay_method = OnlyCash;
		break;
	case 4:
		::current_pay_method = OnlyCreditCards;
		break;
	case 5:
		::current_pay_method = OnlyDigitalWallet;
		break;
	case 6:
		::current_pay_method = TogetherCashAndCreditCards;
		break;
	case 7:
		::current_pay_method = UseWithoutPaymentRestrics;
		break;
	default:
		break;
	}
}

void handle_paid_settings() {
	std::cout << "Select payment method:" << std::endl
		<< "1 - The wallet on which there is a large amount of money;" << std::endl
		<< "2 - Payment by all cash that is available;" << std::endl
		<< "3 - Only cash;" << std::endl
		<< "4 - Only credit cards;" << std::endl
		<< "5 - Only digital wallet;" << std::endl
		<< "6 - Cash & Credit cards;" << std::endl
		<< "7 - Use without payment restrics;" << std::endl
		<< std::endl;

	const std::string promt_message = "Type number of payment method: ";
	int payment_id = user_handle_int(promt_message);

	if (payment_id <= 0 || payment_id > 7) {
		std::cout << "Payment method not changed." << std::endl;
	} else {
		select_payment_method_by_id(payment_id);
		std::cout << "Payment method changed." << std::endl;
	}

	const std::string promt_additional_time_message = "Additional paid time value in seconds: ";
	int paid_time_value = user_handle_int(promt_additional_time_message);

	if (paid_time_value >= 0) {
		::paid_time = paid_time_value;
		std::cout << "Successful change." << std::endl;
	}
}

bool save_to_db(const std::string& file_name, std::vector<std::string> values) {
	std::ofstream file(file_name, std::ios_base::app);

	if (!file.is_open()) {
		std::cout << "Sync error. Can`t opent file for write." << std::endl;
		return false;
	}

	file << std::endl;

	for (std::vector<std::string>::iterator it = values.begin(); it != values.end(); ++it) {
		file << *it << '\t';
	}

	file.close();

	return true;
}

void handle_create_product() {
	std::cout << "Creating new product." << std::endl;

	const std::string promt_name_message = "Name: ";
	std::string name = user_handle(promt_name_message);

	const std::string promt_price_message = "Price: ";
	double price = user_handle_double(promt_price_message);

	const std::string promt_amount_message = "Available amount: ";
	int amount = user_handle_int(promt_amount_message);

	std::cout << "Product type." << std::endl
		<< "1 - Default; 2 - Digital; 3 - Service; 4 - Reqire registration." << std::endl;
	const std::string promt_type_message = "Type number: ";
	int product_type_id = user_handle_int(promt_type_message);

	std::vector<std::string> values{
		name,
		std::to_string(price),
		std::to_string(amount),
		std::to_string(product_type_id),
	};

	product_parser(values);

	bool is_success_sync = save_to_db(PRODUCT_DB_FILE_NAME, values);
	if (is_success_sync) {
		std::cout << name << ", successful added to db." << std::endl;
	}
}

void handle_create_user() {
	std::cout << "Registration new user." << std::endl;

	const std::string promt_name_message = "Full name: ";
	std::string name = user_handle(promt_name_message);

	const std::string promt_amount_cash_message = "Cash: ";
	double amount_cash = user_handle_double(promt_amount_cash_message);

	const std::string promt_money_on_card_message = "Money on card: ";
	double amount_on_card = user_handle_double(promt_money_on_card_message);

	const std::string promt_amount_on_digital_wallet_message = "Money on digital wallet: ";
	double amount_digital = user_handle_double(promt_amount_on_digital_wallet_message);

	const std::string promt_wish_list_message = "Product ids (use space as delimeter): ";
	std::string wish_list = user_handle(promt_wish_list_message);

	std::vector<std::string> values{
		name,
		std::to_string(amount_cash),
		std::to_string(amount_on_card),
		std::to_string(amount_digital),
		wish_list
	};

	user_parser(values);

	bool is_success_sync = save_to_db(USERS_DB_FILE_NAME, values);
	if (is_success_sync) {
		std::cout << name << ", successful added to db." << std::endl;
	}
}

void handle_view_product() {
	std::cout << "Available: " << products.size() << " product(s)" << std::endl;

	const std::string promt_message = "Enter the #id of the product whose information you want to receive: ";
	int product_id = user_handle_int(promt_message);
	Product product;

	try {
		product = products.at(product_id);
	} catch (const std::out_of_range&) {
		std::cout << "Product not found with specified id: " << product_id << std::endl;

		return;
	}

	std::cout << "Product name: " << product.name << std::endl
		<< "Price: " << product.price << std::endl
		<< "Product type: " << product.type << std::endl
		<< "Available amount: " << product.amount << std::endl;
}

void handle_view_user() {
	const std::string promt_message = "Enter the #id of the user whose information you want to receive: ";
	int user_id = user_handle_int(promt_message);
	User user;

	try {
		user = users.at(user_id);
	} catch (const std::out_of_range&) {
		std::cout << "User not found with specified id: " << user_id << std::endl;

		return;
	}

	std::cout << "User name: " << user.name << std::endl
		<< "Cash: " << user.amount_cash << std::endl
		<< "Money on card: " << user.amount_on_card << std::endl
		<< "Money on digital wallet: " << user.amount_digital << std::endl
		<< "Product cart size: " << user.products_cart.size() << std::endl;
}

void handle_view_user_cart() {
	const std::string promt_message = "Enter the #id of the user whose wish cart you want to view: ";
	int user_id = user_handle_int(promt_message);

	User user;
	try {
		user = users.at(user_id);
	} catch (const std::out_of_range&) {
		std::cout << "User not found with specified id: " << user_id << std::endl;

		return;
	}

	double total_sum = 0.0;

	for (std::vector<int>::iterator it = user.products_cart.begin();
		it != user.products_cart.end(); ++it) {

		try {
			Product product = products.at(*it);
			total_sum += product.price;

			std::cout << "#" << (*it) << '\t' << product.name << '\t' << product.price << std::endl;
		} catch (const std::out_of_range&) {
			std::cout << "WARN: Product with id: " << *it << ", not available yet." << std::endl;
		}
	}

	std::cout << "Products on cart: " << user.products_cart.size() << std::endl
		<< "Total sum to pay: " << total_sum << std::endl;
}

std::string user_handle_int_or_random(const std::string& message) {
	try {
		int value = user_handle_int(message);
		return std::to_string(value);
	} catch (std::out_of_range&) {
		return "r";
	}
}

void handle_create_configuration() {
	std::cout << "Creating new configuration." << std::endl;

	const std::string promt_name_message = "Name: ";
	std::string name = user_handle(promt_name_message);

	const std::string promt_max_users_message = "Max users in one queue: ";
	std::string max_users_in_queue = user_handle_int_or_random(promt_max_users_message);

	std::cout << "Processing time in seconds." << std::endl
		<< " Use \'0\' if product type not available." << std::endl
		<< "\'r\' for random in-time value." << std::endl;

	const std::string promt_default_processing_time_message = "Default: ";
	std::string default_processing_time = user_handle_int_or_random(promt_default_processing_time_message);

	const std::string promt_digital_processing_time_message = "Digital: ";
	std::string digital_processing_time = user_handle_int_or_random(promt_digital_processing_time_message);

	const std::string promt_service_processing_time_message = "Service: ";
	std::string service_processing_time = user_handle_int_or_random(promt_service_processing_time_message);

	const std::string promt_registration_processing_time_message = "Require registration: ";
	std::string registration_processing_time = user_handle_int_or_random(promt_registration_processing_time_message);

	const std::string promt_effectivity_rate_message = "Effectivity rate: ";
	std::string effectivity_rate = user_handle_int_or_random(promt_effectivity_rate_message);

	std::vector<std::string> values{
		name,
		max_users_in_queue,
		default_processing_time,
		digital_processing_time,
		service_processing_time,
		registration_processing_time,
		effectivity_rate
	};

	configuration_parser(values);

	bool is_success_sync = save_to_db(CONFIGURATION_DB_FILE_NAME, values);
	if (is_success_sync) {
		std::cout << name << ", successful added to db." << std::endl;
	}
}

void print_detail_configuration_info(Configuration cfg) {
	std::cout << "Name: " << cfg.name << std::endl
		<< "" << cfg.max_users_in_queue << std::endl
		<< "Precessing time: " << std::endl
		<< "Default: " << cfg.default_processing_time << std::endl
		<< "Digital: " << cfg.digital_processing_time << std::endl
		<< "Service: " << cfg.service_processing_time << std::endl
		<< "Registration: " << cfg.registration_processing_time << std::endl
		<< "Effectivity rate: " << cfg.effectivity_rate << std::endl;
}

void handle_view_configuration() {
	show_available_configuration_list();

	std::string promt_message = "Type name of configuration, to detail view: ";
	std::string configuration_name = user_handle(promt_message);

	try {
		Configuration cfg = get_configuration_by_name(configuration_name);
		print_detail_configuration_info(cfg);
	} catch (std::out_of_range&) {
		std::cout << "Configuration with specified name not found!" << std::endl;
	}
}

void handle_application_exit() {
	std::cout << "Exit...";
	exit(0);
}

void execute_command(const std::string& command) {
	if (command == COMMAND_HELP) {
		handle_help();
	} else if (command == COMMAND_ABOUT) {
		handle_about();
	} else if (command == COMMAND_START) {
		handle_start();
	} else if (command == COMMAND_CURRENCY) {
		handle_currency();
	} else if (command == COMMAND_PAID_SETTINGS) {
		handle_paid_settings();
	} else if (command == COMMAND_CREATE_PRODUCT) {
		handle_create_product();
	} else if (command == COMMAND_CREATE_USER) {
		handle_create_user();
	} else if (command == COMMAND_VIEW_PRODUCT) {
		handle_view_product();
	} else if (command == COMMAND_VIEW_USER) {
		handle_view_user();
	} else if (command == COMMAND_VIEW_USER_CART) {
		handle_view_user_cart();
	} else if (command == COMMAND_CREATE_CONFIGURATION) {
		handle_create_configuration();
	} else if (command == COMMAND_VIEW_CONFIGURATION) {
		handle_view_configuration();
	} else if (command == COMMAND_EXIT) {
		handle_application_exit();
	}
}

void main_loop() {
	execute_command(COMMAND_HELP);

	while (true) {
		std::string new_command = user_input("$: ");
		execute_command(new_command);
	}
}

int main() {
	time_t initial_time = time(NULL);
	std::srand((unsigned int)initial_time);
	std::cout << std::fixed << std::setprecision(2);

	load_data();
	main_loop();

	std::cin.get();

	return 0;
}
