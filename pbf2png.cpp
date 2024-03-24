#include <QImage>
#include <QImageReader>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QtConcurrent>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QString>
#include <QDebug>

class App : public QGuiApplication
{
public:
	App(int &argc, char **argv);

	int run();

private:
	static void render(QFileInfo &fi);

	static bool _hidpi;
	// add lopdi 256 x 256 pixels
	static bool _lodpi;
	static QRegularExpression _re;
	static QDir _outdir;
};

bool App::_hidpi = false;
// add _lopdi
bool App::_lodpi = false;
QRegularExpression App::_re;
QDir App::_outdir;

void App::render(QFileInfo &fi)
{
	QImage image;
	QRegularExpressionMatch match = _re.match(fi.baseName());
	// change by new zoom (zoom is in the upper folder name
	// QByteArray zoom(match.captured(1).toLatin1());

	QString findzoom = fi.path();
	int last= findzoom.lastIndexOf("/");
	int first= findzoom.lastIndexOf("/",last-1);
	QString txtzoom = findzoom.mid(first+1,last-first-1);
	QByteArray zoom(txtzoom.toLatin1());

	QImageReader reader(fi.absoluteFilePath(), zoom);
	if (_hidpi)
		reader.setScaledSize(QSize(1024, 1024));
 	// add 
	if (_lodpi)
		reader.setScaledSize(QSize(256, 256));

	if (reader.read(&image)) {
		QString outfile(_outdir.absoluteFilePath(fi.completeBaseName() + ".png"));
		if (!image.save(outfile, "PNG")) {
			QTextStream err(stderr);
			err << outfile << ": error saving image\n";
		}
	} else {
		QTextStream err(stderr);
		err << fi.absoluteFilePath() << ": " << reader.errorString() << "\n";
	}
}

App::App(int &argc, char **argv) : QGuiApplication(argc, argv)
{
	setApplicationName("pbf2png");
}

int App::run()
{
	QCommandLineParser parser;
	QCommandLineOption hidpi("H", "Create HIDPI images");
	//add
	QCommandLineOption lodpi("L", "Create LODPI iages");
	QCommandLineOption mask("M", "Tile name mask", "mask", "([0-9]+).*");
	QCommandLineOption outdir("O", "Output directory", "dir");
	parser.setApplicationDescription("Create PNG tiles from PBF(MVT) tiles");
	parser.addHelpOption();
	parser.addOption(hidpi);
	//add
	parser.addOption(lodpi);
	parser.addOption(mask);
	parser.addOption(outdir);
	parser.addPositionalArgument("DIR", "PBF Tiles directory");
	QTextStream err(stderr);

	if (!parser.parse(arguments())) {
		err << parser.errorText() << "\n";
		return -1;
	}

	const QStringList args = parser.positionalArguments();
	if (!args.size()) {
		err << parser.helpText();
		return -1;
	}
	QDir dir(args.at(0));
	if (!dir.exists()) {
		err << args.at(0) << ": No such directory\n";
		return -1;
	}
	if (!parser.value(outdir).isNull()) {
		_outdir = QDir(parser.value(outdir));
		if (!_outdir.exists()) {
			if (!_outdir.mkpath(".")) {
				err << parser.value(outdir) << ": error creating directory\n";
				return -1;
			}
		}
	} else
		_outdir = dir;

	QFileInfoList list(dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files));

	_re.setPattern(parser.value(mask));
	_hidpi = parser.isSet(hidpi);
	//add
	_lodpi = parser.isSet(lodpi);

	QFuture<void> future = QtConcurrent::map(list, render);
	future.waitForFinished();

	return 0;
}

int main(int argc, char *argv[])
{
	App a(argc, argv);
	return a.run();
}
