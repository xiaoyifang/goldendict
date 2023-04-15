#include "utils.hh"
#include <QDir>
#include <QPalette>
#include <QStyle>

QString Utils::Path::combine(const QString& path1, const QString& path2)
{
  return QDir::cleanPath(path1 + QDir::separator() + path2);
}

QString Utils::Url::getSchemeAndHost( QUrl const & url )
{
  auto _url = url.url();
  auto index = _url.indexOf("://");
  auto hostEndIndex = _url.indexOf("/",index+3);
  return _url.mid(0,hostEndIndex);
}

void Utils::Widget::setNoResultColor(QWidget * widget, bool noResult)
{
  if( noResult ) {
    QPalette pal( widget->palette() );
    //    #febb7d
    QRgb rgb = 0xfebb7d;
    pal.setColor( QPalette::Base, QColor( rgb ) );
    widget->setAutoFillBackground( true );
    widget->setPalette( pal );
  }
  else {
    QPalette pal( widget->style()->standardPalette() );
    widget->setAutoFillBackground( true );
    widget->setPalette( pal );
  }
}

std::string Utils::Html::getHtmlCleaner()
{
  return R"(</font></font></font></font></font></font></font></font></font></font></font></font>
                     </b></b></b></b></b></b></b></b>
                     </i></i></i></i></i></i></i></i>
                     </a></a></a></a></a></a></a></a>)";
}
