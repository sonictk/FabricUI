//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include <FabricUI/FCurveEditor/RuledGraphicsView.h>
#include <FabricUI/FCurveEditor/Ruler.h>
#include <QGraphicsView>
#include <QLayout>
#include <QScrollBar>
#include <FabricUI/Util/QtSignalsSlots.h>

#include <cmath>
#include <qevent.h>
#include <QDebug>
#include <QTimer>

using namespace FabricUI::FCurveEditor;

class RuledGraphicsView::GraphicsView : public QGraphicsView
{
  typedef QGraphicsView Parent;
  RuledGraphicsView* m_parent;
  enum State { PANNING, SELECTING, NOTHING } m_state;
  QRectF m_selectionRect; // in scene space
  QPoint m_lastMousePos; // in widget space
public:
  GraphicsView( RuledGraphicsView* parent )
    : m_parent( parent )
    , m_state( NOTHING )
  {
    this->setDragMode( QGraphicsView::NoDrag ); // Reimplementing it ourself to support both panning and selection
    this->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    this->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  }
protected:
  // HACK ? (move the parent's handler here instead ?)
  void wheelEvent( QWheelEvent * e ) FTL_OVERRIDE { return e->ignore(); }
  void drawBackground( QPainter *, const QRectF & ) FTL_OVERRIDE;
  void mousePressEvent( QMouseEvent *event ) FTL_OVERRIDE
  {
    m_state = NOTHING;
    if( event->button() == Qt::MiddleButton || event->modifiers().testFlag( Qt::AltModifier ) )
    {
      m_lastMousePos = event->pos();
      m_state = PANNING;
      return event->accept();
    }

    Parent::mousePressEvent( event ); // passing the event to the scene
    if( event->isAccepted() )
      return;

    if( m_parent->m_rectangleSelectionEnabled &&
      event->button() == Qt::LeftButton )
    {
      m_state = SELECTING;
      m_selectionRect.setTopLeft( this->mapToScene( event->pos() ) );
      m_selectionRect.setSize( QSizeF( 0, 0 ) );
    }
  }
  void mouseMoveEvent( QMouseEvent *event ) FTL_OVERRIDE
  {
    switch( m_state )
    {
    case PANNING:
    {
      QScrollBar *hBar = horizontalScrollBar();
      QScrollBar *vBar = verticalScrollBar();
      QPoint delta = event->pos() - m_lastMousePos;
      m_lastMousePos = event->pos();
      hBar->setValue( hBar->value() + ( isRightToLeft() ? delta.x() : -delta.x() ) );
      vBar->setValue( vBar->value() - delta.y() );
      m_parent->updateRulersRange();
    } break;
    case SELECTING:
    {
      m_selectionRect.setBottomRight( this->mapToScene( event->pos() ) );
      this->update();
    } break;
    case NOTHING: break;
    }
    Parent::mouseMoveEvent( event );
  }
  void mouseReleaseEvent( QMouseEvent *event ) FTL_OVERRIDE
  {
    if( m_state == SELECTING )
    {
      this->update();
      emit m_parent->rectangleSelectReleased( m_selectionRect );
    }
    m_state = NOTHING;
    Parent::mouseReleaseEvent( event );
  }
  void drawForeground( QPainter * p, const QRectF & r ) FTL_OVERRIDE
  {
    if( m_state == SELECTING )
    {
      QColor col( 0, 128, 255 );
      col.setAlpha( 128 );
      QPen pen( col );
      pen.setCosmetic( true );
      p->setPen( pen );
      p->drawRect( m_selectionRect );
      col.setAlpha( 32 );
      p->fillRect( m_selectionRect, col );
    }
  }
};

QGraphicsView* RuledGraphicsView::view() { return m_view; }

class RuledGraphicsView::Ruler : public FabricUI::FCurveEditor::Ruler
{
  typedef FabricUI::FCurveEditor::Ruler Parent;
  RuledGraphicsView* m_parent;
  bool m_isVertical;

public:
  Ruler( RuledGraphicsView* parent, bool isVertical )
    : Parent( isVertical ? Qt::Vertical : Qt::Horizontal )
    , m_parent( parent )
    , m_isVertical( isVertical )
  {}

protected:
  void wheelEvent( QWheelEvent * e ) FTL_OVERRIDE
  {
    m_parent->wheelEvent(
      m_isVertical ? 0 : e->delta(),
      m_isVertical ? e->delta() : 0,
      m_parent->m_view->mapToScene( m_parent->m_view->mapFromGlobal( e->globalPos() ) )
    );
    e->accept();
  }
  void enterEvent( QEvent *event ) FTL_OVERRIDE
  {
    this->setCursor( m_isVertical ? Qt::SizeVerCursor : Qt::SizeHorCursor );
  }
  void leaveEvent( QEvent *event ) FTL_OVERRIDE { this->unsetCursor(); }
};

RuledGraphicsView::RuledGraphicsView()
  : m_view( new GraphicsView( this ) )
  , m_scrollSpeed( 1 / 800.0f )
  , m_zoomOnCursor( true )
  , m_smoothZoom( true )
  , m_targetScale( QPointF( 1E2, 1E2 ) )
  , m_timer( new QTimer( this ) )
{
  QGridLayout* lay = new QGridLayout();
  lay->setSpacing( 0 ); lay->setMargin( 0 );

  this->enableRectangleSelection( true );

  // HACK / TODO : remove
  this->setStyleSheet( "background-color: #222; border: #000;" );

  lay->addWidget( m_view, 0, 1 );

  m_vRuler = new Ruler( this, true );
  m_vRuler->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding ) );
  lay->addWidget( m_vRuler, 0, 0 );

  m_hRuler = new Ruler( this, false );
  m_hRuler->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
  lay->addWidget( m_hRuler, 1, 1 );
  this->setRulersSize( 24 );

  this->setLayout( lay );

  QOBJECT_CONNECT( m_timer, SIGNAL, QTimer, timeout, ( ), this, SLOT, RuledGraphicsView, tick, ( ) );
  m_timer->setInterval( 16 );
  if( m_smoothZoom )
    m_timer->start();

  this->setTopToBottomY( false );
}

void RuledGraphicsView::setRulersSize( const size_t s )
{
  m_rulersSize = s;
  m_vRuler->setFixedWidth( s );
  m_hRuler->setFixedHeight( s );
}

bool RuledGraphicsView::topToBottomY() const
{
  return ( m_view->matrix().m22() >= 0 );
}

void RuledGraphicsView::setTopToBottomY( bool topToBottom )
{
  if( this->topToBottomY() != topToBottom )
  {
    this->view()->scale( 1, -1 );
    this->updateRulersRange();
  }
}

void RuledGraphicsView::wheelEvent( QWheelEvent * e )
{
  this->wheelEvent( e->delta(), e->delta(), m_view->mapToScene( m_view->mapFromGlobal( e->globalPos() ) ) );
}

void RuledGraphicsView::centeredScale( qreal x, qreal y )
{
  if( m_zoomOnCursor )
  {
    // The rule is that the scaling center must have the same pixel
    // coordinates before and after the scaling
    QPoint viewCenter = QPoint( m_view->width(), m_view->height() ) / 2;
    QPoint scalingCenterPxOffset = m_view->mapFromScene( m_scalingCenter ) - viewCenter;
    m_view->centerOn( m_scalingCenter );
    m_view->scale( x, y );
    QPoint newScalingCenterPxOffset = m_view->mapFromScene( m_scalingCenter ) - viewCenter;
    m_view->centerOn( m_view->mapToScene( viewCenter + ( newScalingCenterPxOffset - scalingCenterPxOffset ) ) );
  }
  else
    m_view->scale( x, y );

  updateRulersRange();
}

void RuledGraphicsView::wheelEvent( int xDelta, int yDelta, QPointF scalingCenter )
{
  m_scalingCenter = scalingCenter;
  qreal sX = ( 1 + xDelta * m_scrollSpeed );
  qreal sY = ( 1 + yDelta * m_scrollSpeed );
  if( m_smoothZoom )
  {
    m_targetScale.setX( m_targetScale.x() * sX );
    m_targetScale.setY( m_targetScale.y() * sY );
    m_timer->start();
    tick();
  }
  else
    centeredScale( sX, sY );
}

void RuledGraphicsView::fitInView( const QRectF r )
{
  this->view()->fitInView( r );
  m_targetScale = QPointF( m_view->matrix().m11(), m_view->matrix().m22() );
  this->updateRulersRange();
}

void RuledGraphicsView::updateRulersRange()
{
  QRectF vrect = m_view->mapToScene( m_view->viewport()->geometry() ).boundingRect();
  if( m_view->matrix().m22() < 0 )
    m_vRuler->setRange( vrect.bottom(), vrect.top() );
  else
    m_vRuler->setRange( vrect.top(), vrect.bottom() );

  QRectF hrect = m_view->mapToScene( m_view->viewport()->geometry() ).boundingRect();
  m_hRuler->setRange( hrect.left(), hrect.right() );
}

void RuledGraphicsView::resizeEvent( QResizeEvent * e )
{
  QWidget::resizeEvent( e );
  this->updateRulersRange();
}

void RuledGraphicsView::tick()
{
  QPointF currentScale = QPointF( m_view->matrix().m11(), m_view->matrix().m22() );
  if(
    std::abs( std::log( currentScale.x() ) - std::log( m_targetScale.x() ) ) +
    std::abs( std::log( std::abs(currentScale.y()) ) - std::log( std::abs(m_targetScale.y()) ) )
  > 0.01 ) // If we are close enough to the target, we stop the animation
  {
    const qreal ratio = 0.2; // TODO : property
    QPointF newScale(
      ( 1 - ratio ) * currentScale.x() + ratio * m_targetScale.x(),
      ( 1 - ratio ) * currentScale.y() + ratio * m_targetScale.y()
    );
    this->centeredScale(
      newScale.x() / currentScale.x(),
      newScale.y() / currentScale.y()
    );
  }
  else
    m_timer->stop();
}

void RuledGraphicsView::GraphicsView::drawBackground( QPainter * p, const QRectF & r )
{
  // Background (HACK/TODO : remove)
  p->fillRect( r, QColor( 64, 64, 64 ) );

  QRect wr = this->viewport()->geometry(); // widget viewRect
  QRectF sr = this->mapToScene( wr ).boundingRect(); // scene viewRect

  // Grid
  for( int o = 0; o < 2; o++ ) // 2 orientations
  {
    const Qt::Orientation ori = o == 0 ? Qt::Vertical : Qt::Horizontal;

    qreal size = ( ori == Qt::Vertical ? sr.height() : sr.width() );
    qreal minU = ( ori == Qt::Vertical ? sr.top() : sr.left() );
    qreal maxU = ( ori == Qt::Vertical ? sr.bottom() : sr.right() );

    const qreal logScale = 2.0f; // TODO : property
    qreal viewFactor = 8.0f / size; // TODO : property
    if( ori == Qt::Vertical )
      viewFactor *= qreal( wr.height() ) / wr.width();

    qreal minFactor = std::pow( logScale, std::floor( std::log( viewFactor ) / std::log( logScale ) ) );
    qreal maxFactor = 150.0f / size; // TODO : Q_PROPERTY
    for( qreal factor = minFactor; factor < maxFactor; factor *= logScale )
    {
      QPen pen;
      // Pen width
      {
        // We use a cosmetic pen here, because when we tried a "scene-space"
        // pen, precision errors were making thin lines be inconsistently invisible
        // for the same factor
        pen.setCosmetic( true );
        qreal pwidth = ( 2 * viewFactor ) / factor; // TODO : property
        qreal palpha = 255;
        if( pwidth < 1 )
        {
          palpha = pwidth * 255;
          pwidth = 1;
        }
        pen.setWidthF( pwidth );
        pen.setColor( QColor( 32, 32, 32, int(palpha) ) ); // TODO : qss color
      }
      p->setPen( pen );
      for( qreal i = std::floor( factor * minU ); i < factor * maxU; i++ )
        if( ori == Qt::Horizontal )
          p->drawLine( QPointF( i / factor, sr.top() ), QPointF( i / factor, sr.bottom() ) );
        else
          p->drawLine( QPointF( sr.left(), i / factor ), QPointF( sr.right(), i / factor ) );
    }
  }
}
